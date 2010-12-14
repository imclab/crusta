#include <GL/VruiGlew.h> //must be included before gl.h

#include <crusta/CrustaApp.h>

#include <sstream>

#if __APPLE__
#include <GDAL/ogr_api.h>
#include <GDAL/ogrsf_frmts.h>
#else
#include <ogr_api.h>
#include <ogrsf_frmts.h>
#endif

#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/DropdownBox.h>
#include <GLMotif/Label.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Menu.h>
#include <GLMotif/PaletteEditor.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/ScrolledListBox.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/TextField.h>
#include <GLMotif/WidgetManager.h>
#include <GL/GLColorMap.h>
#include <GL/GLContextData.h>
#include <Vrui/Lightsource.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/Tools/LocatorTool.h>
#include <Vrui/Viewer.h>
#include <Vrui/Vrui.h>

#include <crusta/ColorMapper.h>
#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/QuadTerrain.h>
#include <crusta/SurfaceProbeTool.h>
#include <crusta/SurfaceTool.h>


#include <Geometry/Geoid.h>
#include <Misc/FunctionCalls.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Tools/SurfaceNavigationTool.h>

#if CRUSTA_ENABLE_DEBUG
#include <crusta/DebugTool.h>
#endif //CRUSTA_ENABLE_DEBUG


BEGIN_CRUSTA

CrustaApp::
CrustaApp(int& argc, char**& argv, char**& appDefaults) :
    Vrui::Application(argc, argv, appDefaults),
    newVerticalScale(1.0), dataDialog(NULL), dataDemFile(NULL),
    dataColorFile(NULL), enableSun(false),
    viewerHeadlightStates(new bool[Vrui::getNumViewers()]),
    sun(0), sunAzimuth(180.0), sunElevation(45.0),
    paletteEditor(new GLMotif::PaletteEditor), colorMapSettings(paletteEditor)
{
    paletteEditor->getColorMapEditor()->getColorMapChangedCallbacks().add(
        this, &CrustaApp::changeColorMapCallback);
    paletteEditor->getRangeEditor()->getRangeChangedCallbacks().add(
        this, &CrustaApp::changeColorMapRangeCallback);

    typedef std::vector<std::string> Strings;

    Strings dataNames;
    Strings settingsNames;
    for (int i=1; i<argc; ++i)
    {
        std::string token = std::string(argv[i]);
        if (token == std::string("-settings"))
            settingsNames.push_back(argv[++i]);
        else
            dataNames.push_back(token);

    }

    crusta = new Crusta;
    crusta->init(settingsNames);
    //load data passed through command line?
    crusta->load(dataNames);

    /* Create the sun lightsource: */
    sun=Vrui::getLightsourceManager()->createLightsource(false);
    updateSun();

    /* Save all viewers' headlight enable states: */
    for(int i=0;i<Vrui::getNumViewers();++i)
    {
        viewerHeadlightStates[i] =
            Vrui::getViewer(i)->getHeadlight().isEnabled();
    }

    specularSettings.setupComponent(crusta);

    produceMainMenu();
    produceVerticalScaleDialog();
    produceLightingDialog();

    resetNavigationCallback(NULL);

///\todo fix the loading UI
//LoadDataWidget.setFileNames(dataNames)
    dataDemFile->setLabel("");
    dataColorFile->setLabel("");

#if CRUSTA_ENABLE_DEBUG
    DebugTool::init();
#endif //CRUSTA_ENABLE_DEBUG
}

CrustaApp::
~CrustaApp()
{
    delete popMenu;
    /* Delete the viewer headlight states: */
    delete[] viewerHeadlightStates;

    crusta->shutdown();
    delete crusta;
}


void CrustaApp::Dialog::
createMenuEntry(GLMotif::Container* menu)
{
    init();

    parentMenu = menu;

    GLMotif::ToggleButton* toggle = new GLMotif::ToggleButton(
        (name+"Toggle").c_str(), parentMenu, label.c_str());
    toggle->setToggle(false);
    toggle->getValueChangedCallbacks().add(
        this, &CrustaApp::Dialog::showCallback);
}

void CrustaApp::Dialog::
init()
{
    dialog = new GLMotif::PopupWindow(
        (name+"Dialog").c_str(), Vrui::getWidgetManager(), label.c_str());
}

void CrustaApp::Dialog::
showCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if(cbData->set)
    {
        //open the dialog at the same position as the menu:
        Vrui::getWidgetManager()->popupPrimaryWidget(dialog,
            Vrui::getWidgetManager()->calcWidgetTransformation(parentMenu));
    }
    else
    {
        //close the dialog:
        Vrui::popdownPrimaryWidget(dialog);
    }
}


CrustaApp::SpecularSettingsDialog::
SpecularSettingsDialog() :
    CrustaComponent(NULL), colorPicker("SpecularSettingsColorPicker",
                                       Vrui::getWidgetManager(),
                                       "Pick Specular Color")
{
    name  = "SpecularSettings";
    label = "Specular Reflectance Settings";
}

void CrustaApp::SpecularSettingsDialog::
init()
{
    Dialog::init();

    const GLMotif::StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();

    colorPicker.getColorPicker()->getColorChangedCallbacks().add(
            this, &CrustaApp::SpecularSettingsDialog::colorChangedCallback);


    GLMotif::RowColumn* root = new GLMotif::RowColumn(
        "Root", dialog, false);

    GLMotif::RowColumn* colorRoot = new GLMotif::RowColumn(
        "ColorRoot", root, false);

    Color sc = SETTINGS->terrainSpecularColor;
    new GLMotif::Label("SSColor", colorRoot, "Color: ");
    colorButton = new GLMotif::Button("SSColorButton", colorRoot, "");
    colorButton->setBackgroundColor(GLMotif::Color(sc[0], sc[1], sc[2], sc[3]));
    colorButton->getSelectCallbacks().add(
        this, &CrustaApp::SpecularSettingsDialog::colorButtonCallback);

    colorRoot->setNumMinorWidgets(2);
    colorRoot->manageChild();

    GLMotif::RowColumn* shininessRoot = new GLMotif::RowColumn(
        "ShininessRoot", root, false);

    float shininess = SETTINGS->terrainShininess;
    new GLMotif::Label("SSShininess", shininessRoot, "Shininess: ");
    GLMotif::Slider* slider = new GLMotif::Slider(
        "SSShininessSlider", shininessRoot, GLMotif::Slider::HORIZONTAL,
        10.0 * style->fontHeight);
    slider->setValue(log(shininess)/log(2));
    slider->setValueRange(0.00f, 7.0f, 0.00001f);
    slider->getValueChangedCallbacks().add(
        this, &CrustaApp::SpecularSettingsDialog::shininessChangedCallback);
    shininessField = new GLMotif::TextField(
        "SSShininessField", shininessRoot, 7);
    shininessField->setPrecision(4);
    shininessField->setValue(shininess);

    shininessRoot->setNumMinorWidgets(3);
    shininessRoot->manageChild();


    root->setNumMinorWidgets(2);
    root->manageChild();
}

void CrustaApp::SpecularSettingsDialog::
colorButtonCallback(
    GLMotif::Button::SelectCallbackData* cbData)
{
    GLMotif::WidgetManager* manager = Vrui::getWidgetManager();
    if (!manager->isVisible(&colorPicker))
    {
        //bring up the color picker
        colorPicker.getColorPicker()->setCurrentColor(
            SETTINGS->terrainSpecularColor);
        manager->popupPrimaryWidget(
            &colorPicker, manager->calcWidgetTransformation(cbData->button));
    }
    else
    {
        //close the color picker
        Vrui::popdownPrimaryWidget(&colorPicker);
    }
}

void CrustaApp::SpecularSettingsDialog::
colorChangedCallback(
        GLMotif::ColorPicker::ColorChangedCallbackData* cbData)
{
    const GLMotif::Color& c = cbData->newColor;
    colorButton->setBackgroundColor(c);
    crusta->setTerrainSpecularColor(Color(c[0], c[1], c[2], c[3]));
}

void CrustaApp::SpecularSettingsDialog::
shininessChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    float shininess = pow(2, cbData->value) - 1.0f;
    crusta->setTerrainShininess(shininess);
    shininessField->setValue(shininess);
}

CrustaApp::ColorMapSettingsDialog::
ColorMapSettingsDialog(GLMotif::PaletteEditor* editor) :
    listBox(NULL), paletteEditor(editor)
{
    name  = "ColorMapSettings";
    label = "Color Map Settings";
}

void CrustaApp::ColorMapSettingsDialog::
init()
{
    Dialog::init();

    GLMotif::RowColumn* root = new GLMotif::RowColumn("Root", dialog, false);

    GLMotif::ScrolledListBox* box = new GLMotif::ScrolledListBox(
        "ColorMapListBox", root, GLMotif::ListBox::ALWAYS_ONE, 30, 8);
    listBox = box->getListBox();
    listBox->getValueChangedCallbacks().add(
        this, &ColorMapSettingsDialog::layerChangedCallback);

    GLMotif::ToggleButton* clampButton = new GLMotif::ToggleButton(
        "ColoMapClampButton", root, "Clamp");
    clampButton->getValueChangedCallbacks().add(
        this, &ColorMapSettingsDialog::clampCallback);

    updateLayerList();

    root->setNumMinorWidgets(2);
    root->manageChild();
}


void CrustaApp::ColorMapSettingsDialog::
updateLayerList()
{
    listBox->clear();

    int numLayers = COLORMAPPER->getNumColorMaps();
    for (int i=0; i<numLayers; ++i)
    {
        std::ostringstream oss;
        oss << "Layer " << i;
        listBox->addItem(oss.str().c_str());
    }
    if (numLayers>0)
        listBox->selectItem(0);
}

void CrustaApp::ColorMapSettingsDialog::
layerChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData)
{
    COLORMAPPER->setActiveMap(cbData->newSelectedItem);
    Misc::ColorMap& colorMap =
        COLORMAPPER->getColorMap(cbData->newSelectedItem);

    paletteEditor->getColorMapEditor()->getColorMap() = colorMap;
    paletteEditor->getColorMapEditor()->touch();
}

void CrustaApp::ColorMapSettingsDialog::
clampCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    COLORMAPPER->setClamping(cbData->set);
}



void CrustaApp::
produceMainMenu()
{
    /* Create a popup shell to hold the main menu: */
    popMenu = new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
    popMenu->setTitle("Crusta");

    /* Create the main menu itself: */
    GLMotif::Menu* mainMenu =
    new GLMotif::Menu("MainMenu",popMenu,false);

    /* Data Loading menu entry */
    produceDataDialog();
    GLMotif::Button* dataLoadButton = new GLMotif::Button(
        "DataLoadButton", mainMenu, "Load Data");
    dataLoadButton->getSelectCallbacks().add(
        this, &CrustaApp::showDataDialogCallback);

    /* Create a submenu to toggle texturing the terrain: */
    produceTexturingSubmenu(mainMenu);

    /* Create a button to open or hide the vertical scale adjustment dialog: */
    GLMotif::ToggleButton* showVerticalScaleToggle = new GLMotif::ToggleButton(
        "ShowVerticalScaleToggle", mainMenu, "Vertical Scale");
    showVerticalScaleToggle->setToggle(false);
    showVerticalScaleToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showVerticalScaleCallback);

    /* Create a button to toogle display of the lighting dialog: */
    GLMotif::ToggleButton* lightingToggle = new GLMotif::ToggleButton(
        "LightingToggle", mainMenu, "Light Settings");
    lightingToggle->setToggle(false);
    lightingToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showLightingDialogCallback);

    /* Inject the map management menu entries */
    crusta->getMapManager()->addMenuEntry(mainMenu);

    //color map settings dialog toggle
    colorMapSettings.createMenuEntry(mainMenu);

    /* Create a button to open or hide the palette editor dialog: */
    GLMotif::ToggleButton* showPaletteEditorToggle = new GLMotif::ToggleButton(
        "ShowPaletteEditorToggle", mainMenu, "Palette Editor");
    showPaletteEditorToggle->setToggle(false);
    showPaletteEditorToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showPaletteEditorCallback);

    /* Create settings submenu */
    GLMotif::Popup* settingsMenuPopup =
        new GLMotif::Popup("SettingsMenuPopup", Vrui::getWidgetManager());
    GLMotif::SubMenu* settingsMenu =
        new GLMotif::SubMenu("Settings", settingsMenuPopup, false);

    //line decoration toggle
    GLMotif::ToggleButton* decorateLinesToggle = new GLMotif::ToggleButton(
        "DecorateLinesToggle", settingsMenu, "Decorate Lines");
    decorateLinesToggle->setToggle(SETTINGS->decorateVectorArt);
    decorateLinesToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::decorateLinesCallback);

    //specular settings dialog toggle
    specularSettings.createMenuEntry(settingsMenu);

    /* Create the advanced submenu */
    GLMotif::Popup* advancedMenuPopup =
        new GLMotif::Popup("AdvancedMenuPopup", Vrui::getWidgetManager());
    GLMotif::SubMenu* advancedMenu =
        new GLMotif::SubMenu("Advanced", advancedMenuPopup, false);

    //toogle display of the debugging grid
    GLMotif::ToggleButton* debugGridToggle = new GLMotif::ToggleButton(
        "DebugGridToggle", advancedMenu, "Debug Grid");
    debugGridToggle->setToggle(false);
    debugGridToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::debugGridCallback);

    //toogle display of the debugging sphere
    GLMotif::ToggleButton* debugSpheresToggle = new GLMotif::ToggleButton(
        "DebugSpheresToggle", advancedMenu, "Debug Spheres");
    debugSpheresToggle->setToggle(false);
    debugSpheresToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::debugSpheresCallback);

    advancedMenu->manageChild();
    GLMotif::CascadeButton* advancedMenuCascade = new GLMotif::CascadeButton(
        "AdvancedMenuCascade", settingsMenu, "Advanced");
    advancedMenuCascade->setPopup(advancedMenuPopup);

    settingsMenu->manageChild();
    GLMotif::CascadeButton* settingsMenuCascade = new GLMotif::CascadeButton(
        "SettingsMenuCascade", mainMenu, "Settings");
    settingsMenuCascade->setPopup(settingsMenuPopup);


    /* Navigation reset: */
    GLMotif::Button* resetNavigationButton = new GLMotif::Button(
        "ResetNavigationButton",mainMenu,"Reset Navigation");
    resetNavigationButton->getSelectCallbacks().add(
        this, &CrustaApp::resetNavigationCallback);


    /* Finish building the main menu: */
    mainMenu->manageChild();

    Vrui::setMainMenu(popMenu);
}

void CrustaApp::
produceDataDialog()
{
    dataDialog = new GLMotif::PopupWindow(
        "DataDialog", Vrui::getWidgetManager(), "Load Crusta Data");
    GLMotif::RowColumn* root = new GLMotif::RowColumn(
        "DataRoot", dataDialog, false);

    GLMotif::RowColumn* demRoot = new GLMotif::RowColumn(
        "DataDemRoot", root, false);
    demRoot->setNumMinorWidgets(2);
    dataDemFile = new GLMotif::Label("DataDemLabel", demRoot, "");
    GLMotif::Button* demFileButton = new GLMotif::Button(
        "DataDemFileButton", demRoot, "Elevation");
    demFileButton->getSelectCallbacks().add(
        this, &CrustaApp::loadDemCallback);
    demRoot->manageChild();

    GLMotif::RowColumn* colorRoot = new GLMotif::RowColumn(
        "DataColorRoot", root, false);
    colorRoot->setNumMinorWidgets(2);
    dataColorFile = new GLMotif::Label("DataColorLabel", colorRoot, "");
    GLMotif::Button* colorFileButton = new GLMotif::Button(
        "DataColorFileButton", colorRoot, "Color");
    colorFileButton->getSelectCallbacks().add(
        this, &CrustaApp::loadColorCallback);
    colorRoot->manageChild();

    GLMotif::RowColumn* actionRoot = new GLMotif::RowColumn(
        "DataActionRoot", root, false);
    actionRoot->setNumMinorWidgets(2);
    GLMotif::Button* cancelButton = new GLMotif::Button(
        "DataCancelButton", actionRoot, "Cancel");
    cancelButton->getSelectCallbacks().add(
        this, &CrustaApp::loadDataCancelCallback);
    GLMotif::Button* okButton = new GLMotif::Button(
        "DataOkButton", actionRoot, "Load");
    okButton->getSelectCallbacks().add(
        this, &CrustaApp::loadDataOkCallback);
    actionRoot->manageChild();

    root->setNumMinorWidgets(1);
    root->manageChild();
}

void CrustaApp::
showDataDialogCallback(GLMotif::Button::SelectCallbackData*)
{
    //open the dialog at the same position as the main menu:
    Vrui::getWidgetManager()->popupPrimaryWidget(dataDialog,
        Vrui::getWidgetManager()->calcWidgetTransformation(popMenu));
}

void CrustaApp::
loadDemCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    GLMotif::FileAndFolderSelectionDialog* fileDialog =
        new GLMotif::FileAndFolderSelectionDialog(Vrui::getWidgetManager(),
            "Load Crusta Elevation", 0, NULL);
    fileDialog->getOKCallbacks().add(this,
        &CrustaApp::loadDemFileOkCallback);
    fileDialog->getCancelCallbacks().add(this,
        &CrustaApp::loadDemFileCancelCallback);
    Vrui::getWidgetManager()->popupPrimaryWidget(fileDialog,
        Vrui::getWidgetManager()->calcWidgetTransformation(dataDialog));
}

void CrustaApp::
loadDemFileOkCallback(
    GLMotif::FileAndFolderSelectionDialog::OKCallbackData* cbData)
{
    //save the selected file in the corresponding label
    dataDemFile->setLabel(cbData->selectedFileName.c_str());
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}

void CrustaApp::
loadDemFileCancelCallback(
    GLMotif::FileAndFolderSelectionDialog::CancelCallbackData* cbData)
{
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}

void CrustaApp::
loadColorCallback(GLMotif::Button::SelectCallbackData*)
{
    GLMotif::FileAndFolderSelectionDialog* fileDialog =
        new GLMotif::FileAndFolderSelectionDialog(Vrui::getWidgetManager(),
            "Load Crusta Color", 0, NULL);
    fileDialog->getOKCallbacks().add(this,
        &CrustaApp::loadColorFileOkCallback);
    fileDialog->getCancelCallbacks().add(this,
        &CrustaApp::loadColorFileCancelCallback);
    Vrui::getWidgetManager()->popupPrimaryWidget(fileDialog,
        Vrui::getWidgetManager()->calcWidgetTransformation(dataDialog));
}

void CrustaApp::
loadColorFileOkCallback(
    GLMotif::FileAndFolderSelectionDialog::OKCallbackData* cbData)
{
    //save the selected file in the corresponding label
    dataColorFile->setLabel(cbData->selectedFileName.c_str());
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}

void CrustaApp::
loadColorFileCancelCallback(
    GLMotif::FileAndFolderSelectionDialog::CancelCallbackData* cbData)
{
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}

void CrustaApp::
loadDataOkCallback(GLMotif::Button::SelectCallbackData*)
{
    //load the current data selection
    std::vector<std::string> dataFiles;
    dataFiles.push_back(dataDemFile->getLabel());
    dataFiles.push_back(dataColorFile->getLabel());

    crusta->load(dataFiles);

    colorMapSettings.updateLayerList();

    //close the dialog
    Vrui::popdownPrimaryWidget(dataDialog);
}

void CrustaApp::
loadDataCancelCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    //close the dialog
    Vrui::popdownPrimaryWidget(dataDialog);
}


void CrustaApp::
produceTexturingSubmenu(GLMotif::Menu* mainMenu)
{
    GLMotif::Popup* texturingMenuPopup =
        new GLMotif::Popup("TexturingMenuPopup", Vrui::getWidgetManager());

    GLMotif::SubMenu* texturingMenu =
        new GLMotif::SubMenu("Texturing", texturingMenuPopup, false);

    GLMotif::RadioBox* texturingBox =
        new GLMotif::RadioBox("TexturingBox", texturingMenu, false);

    GLMotif::ToggleButton* modeButton;
    modeButton = new GLMotif::ToggleButton(
        "Untextured", texturingBox, "Untextured");
    modeButton->getSelectCallbacks().add(
        this, &CrustaApp::changeTexturingModeCallback);
    modeButton = new GLMotif::ToggleButton(
        "ColorMap", texturingBox, "Color Map");
    modeButton->getSelectCallbacks().add(
        this, &CrustaApp::changeTexturingModeCallback);
    modeButton = new GLMotif::ToggleButton(
        "Image", texturingBox, "Image");
    modeButton->getSelectCallbacks().add(
        this, &CrustaApp::changeTexturingModeCallback);

    texturingBox->setSelectedToggle(modeButton);
    texturingBox->manageChild();

    texturingMenu->manageChild();

    GLMotif::CascadeButton* texturingMenuCascade =
        new GLMotif::CascadeButton("TexturingMenuCascade", mainMenu,
                                   "Texturing Modes");
    texturingMenuCascade->setPopup(texturingMenuPopup);
}

void CrustaApp::
produceVerticalScaleDialog()
{
    const GLMotif::StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();

    verticalScaleDialog = new GLMotif::PopupWindow(
        "ScaleDialog", Vrui::getWidgetManager(), "Vertical Scale");
    GLMotif::RowColumn* root = new GLMotif::RowColumn(
        "ScaleRoot", verticalScaleDialog, false);
    GLMotif::Slider* slider = new GLMotif::Slider(
        "ScaleSlider", root, GLMotif::Slider::HORIZONTAL,
        10.0 * style->fontHeight);
    verticalScaleLabel = new GLMotif::Label("ScaleLabel", root, "1.0x");

    slider->setValue(0.0);
    slider->setValueRange(-0.5, 2.5, 0.00001);
    slider->getValueChangedCallbacks().add(
        this, &CrustaApp::changeScaleCallback);

    root->setNumMinorWidgets(2);
    root->manageChild();
}

void CrustaApp::
produceLightingDialog()
{
    const GLMotif::StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();
    lightingDialog=new GLMotif::PopupWindow("LightingDialog",
        Vrui::getWidgetManager(), "Light Settings");
    GLMotif::RowColumn* lightSettings=new GLMotif::RowColumn("LightSettings",
        lightingDialog, false);
    lightSettings->setNumMinorWidgets(2);

    /* Create a toggle button and two sliders to manipulate the sun light source: */
    GLMotif::Margin* enableSunToggleMargin=new GLMotif::Margin("SunToggleMargin",lightSettings,false);
    enableSunToggleMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HFILL,GLMotif::Alignment::VCENTER));
    GLMotif::ToggleButton* enableSunToggle=new GLMotif::ToggleButton("SunToggle",enableSunToggleMargin,"Sun Light Source");
    enableSunToggle->setToggle(enableSun);
    enableSunToggle->getValueChangedCallbacks().add(this,&CrustaApp::enableSunToggleCallback);
    enableSunToggleMargin->manageChild();

    GLMotif::RowColumn* sunBox=new GLMotif::RowColumn("SunBox",lightSettings,false);
    sunBox->setOrientation(GLMotif::RowColumn::VERTICAL);
    sunBox->setNumMinorWidgets(2);
    sunBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);

    sunAzimuthTextField=new GLMotif::TextField("SunAzimuthTextField",sunBox,5);
    sunAzimuthTextField->setFloatFormat(GLMotif::TextField::FIXED);
    sunAzimuthTextField->setFieldWidth(3);
    sunAzimuthTextField->setPrecision(0);
    sunAzimuthTextField->setValue(double(sunAzimuth));

    sunAzimuthSlider=new GLMotif::Slider("SunAzimuthSlider",sunBox,GLMotif::Slider::HORIZONTAL,style->fontHeight*10.0f);
    sunAzimuthSlider->setValueRange(0.0,360.0,1.0);
    sunAzimuthSlider->setValue(double(sunAzimuth));
    sunAzimuthSlider->getValueChangedCallbacks().add(this,&CrustaApp::sunAzimuthSliderCallback);

    sunElevationTextField=new GLMotif::TextField("SunElevationTextField",sunBox,5);
    sunElevationTextField->setFloatFormat(GLMotif::TextField::FIXED);
    sunElevationTextField->setFieldWidth(2);
    sunElevationTextField->setPrecision(0);
    sunElevationTextField->setValue(double(sunElevation));

    sunElevationSlider=new GLMotif::Slider("SunElevationSlider",sunBox,GLMotif::Slider::HORIZONTAL,style->fontHeight*10.0f);
    sunElevationSlider->setValueRange(-90.0,90.0,1.0);
    sunElevationSlider->setValue(double(sunElevation));
    sunElevationSlider->getValueChangedCallbacks().add(this,&CrustaApp::sunElevationSliderCallback);

    sunBox->manageChild();
    lightSettings->manageChild();
}

void CrustaApp::
alignSurfaceFrame(Vrui::NavTransform& surfaceFrame)
{
/* Do whatever to the surface frame, but don't change its scale factor: */
    Point3 origin = surfaceFrame.getOrigin();
    if (origin == Point3::origin)
        origin = Point3(0.0, 1.0, 0.0);

    SurfacePoint surfacePoint = crusta->snapToSurface(origin);

    //misuse the Geoid just to build a surface tangent frame
    Geometry::Geoid<double> geoid(SETTINGS->globeRadius, 0.0);
    Point3 lonLatEle = geoid.cartesianToGeodetic(surfacePoint.position);
    Geometry::Geoid<double>::Frame frame =
        geoid.geodeticToCartesianFrame(lonLatEle);
    surfaceFrame = Vrui::NavTransform(
        frame.getTranslation(), frame.getRotation(), surfaceFrame.getScaling());
}


void CrustaApp::
changeTexturingModeCallback(
    GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if (cbData->set)
    {
        const char* button = cbData->toggle->getName();
        if (strcmp(button, "Untextured") == 0)
            crusta->setTexturingMode(0);
        else if (strcmp(button, "ColorMap") == 0)
            crusta->setTexturingMode(1);
        else
            crusta->setTexturingMode(2);
    }
}

void CrustaApp::
changeColorMapCallback(
    GLMotif::ColorMapEditor::ColorMapChangedCallbackData* cbData)
{
    int mapIndex = COLORMAPPER->getActiveMap();
    if (mapIndex < 0)
        return;

    Misc::ColorMap& colorMap = COLORMAPPER->getColorMap(mapIndex);
    colorMap = cbData->editor->getColorMap();
    COLORMAPPER->touchColor(mapIndex);
}

void CrustaApp::
changeColorMapRangeCallback(
    GLMotif::RangeEditor::RangeChangedCallbackData* cbData)
{
    int mapIndex = COLORMAPPER->getActiveMap();
    if (mapIndex < 0)
        return;

    Misc::ColorMap& colorMap = COLORMAPPER->getColorMap(mapIndex);
    colorMap.setValueRange(Misc::ColorMap::ValueRange(cbData->min,cbData->max));
    COLORMAPPER->touchRange(mapIndex);
}


void CrustaApp::
showVerticalScaleCallback(
    GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if (cbData->set)
    {
        //open the dialog at the same position as the main menu:
        Vrui::getWidgetManager()->popupPrimaryWidget(verticalScaleDialog,
            Vrui::getWidgetManager()->calcWidgetTransformation(popMenu));
    }
    else
    {
        //close the dialog
        Vrui::popdownPrimaryWidget(verticalScaleDialog);
    }
}

void CrustaApp::
changeScaleCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    double newVerticalScale = pow(10, cbData->value);
    crusta->setVerticalScale(newVerticalScale);

    std::ostringstream oss;
    oss.precision(2);
    oss << newVerticalScale << "x";
    verticalScaleLabel->setLabel(oss.str().c_str());
}


void CrustaApp::
showLightingDialogCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if(cbData->set)
    {
        //open the dialog at the same position as the main menu:
        Vrui::getWidgetManager()->popupPrimaryWidget(lightingDialog,
            Vrui::getWidgetManager()->calcWidgetTransformation(popMenu));
    }
    else
    {
        //close the dialog:
        Vrui::popdownPrimaryWidget(lightingDialog);
    }
}

void CrustaApp::
updateSun()
{
    /* Enable or disable the light source: */
    if(enableSun)
        sun->enable();
    else
        sun->disable();

    /* Compute the light source's direction vector: */
    Vrui::Scalar z=Math::sin(Math::rad(sunElevation));
    Vrui::Scalar xy=Math::cos(Math::rad(sunElevation));
    Vrui::Scalar x=xy*Math::sin(Math::rad(sunAzimuth));
    Vrui::Scalar y=xy*Math::cos(Math::rad(sunAzimuth));
    sun->getLight().position=GLLight::Position(GLLight::Scalar(x),GLLight::Scalar(y),GLLight::Scalar(z),GLLight::Scalar(0));
}

void CrustaApp::
enableSunToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    /* Set the sun enable flag: */
    enableSun=cbData->set;

    /* Enable/disable all viewers' headlights: */
    if(enableSun)
    {
        for(int i=0;i<Vrui::getNumViewers();++i)
            Vrui::getViewer(i)->setHeadlightState(false);
    }
    else
    {
        for(int i=0;i<Vrui::getNumViewers();++i)
            Vrui::getViewer(i)->setHeadlightState(viewerHeadlightStates[i]);
    }

    /* Update the sun light source: */
    updateSun();

    Vrui::requestUpdate();
}

void CrustaApp::
sunAzimuthSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    /* Update the sun azimuth angle: */
    sunAzimuth=Vrui::Scalar(cbData->value);

    /* Update the sun azimuth value label: */
    sunAzimuthTextField->setValue(double(cbData->value));

    /* Update the sun light source: */
    updateSun();

    Vrui::requestUpdate();
}

void CrustaApp::
sunElevationSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    /* Update the sun elevation angle: */
    sunElevation=Vrui::Scalar(cbData->value);

    /* Update the sun elevation value label: */
    sunElevationTextField->setValue(double(cbData->value));

    /* Update the sun light source: */
    updateSun();

    Vrui::requestUpdate();
}



void CrustaApp::
showPaletteEditorCallback(
    GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if(cbData->set)
    {
        //open the dialog at the same position as the main menu:
        Vrui::getWidgetManager()->popupPrimaryWidget(paletteEditor,
            Vrui::getWidgetManager()->calcWidgetTransformation(popMenu));
    }
    else
    {
        //close the dialog:
        Vrui::popdownPrimaryWidget(paletteEditor);
    }
}


void CrustaApp::
decorateLinesCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    crusta->setDecoratedVectorArt(cbData->set);
}

void CrustaApp::
debugGridCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    QuadTerrain::displayDebuggingGrid = cbData->set;
}

void CrustaApp::
debugSpheresCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    QuadTerrain::displayDebuggingBoundingSpheres = cbData->set;
}

void CrustaApp::
surfaceSamplePickedCallback(SurfaceProbeTool::SampleCallbackData* cbData)
{
    int activeMap = COLORMAPPER->getActiveMap();
    if (activeMap == -1)
        return;

///\todo make this more generic not just for layerf
    LayerDataf::Type sample =
        DATAMANAGER->sampleLayerf(activeMap, cbData->surfacePoint);
    if (sample == DATAMANAGER->getLayerfNodata())
        return;

    if (cbData->numSamples == 1)
        paletteEditor->getRangeEditor()->shift(sample);
    else if (cbData->sampleId == 1)
        paletteEditor->getRangeEditor()->setMin(sample);
    else
        paletteEditor->getRangeEditor()->setMax(sample);
}

void CrustaApp::
resetNavigationCallback(Misc::CallbackData* cbData)
{
    /* Reset the Vrui navigation transformation: */
    Vrui::setNavigationTransformation(Vrui::Point(0),1.5*SETTINGS->globeRadius);
    Vrui::concatenateNavigationTransformation(Vrui::NavTransform::translate(
        Vrui::Vector(0,1,0)));
}



void CrustaApp::
frame()
{
    crusta->frame();
}

void CrustaApp::
display(GLContextData& contextData) const
{
    //make sure the glew context has been made current before any other display
    VruiGlew::enable(contextData);
    //let crusta do its thing
    crusta->display(contextData);
}


void CrustaApp::
toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
{
    /* Check if the new tool is a surface navigation tool: */
    Vrui::SurfaceNavigationTool* surfaceNavigationTool =
        dynamic_cast<Vrui::SurfaceNavigationTool*>(cbData->tool);
    if (surfaceNavigationTool != NULL)
    {
        /* Set the new tool's alignment function: */
        surfaceNavigationTool->setAlignFunction(
            Misc::createFunctionCall<Vrui::NavTransform&,CrustaApp>(
                this,&CrustaApp::alignSurfaceFrame));
    }

    //all crusta components get the crusta instance assigned
    CrustaComponent* component = dynamic_cast<CrustaComponent*>(cbData->tool);
    if (component != NULL)
        component->setupComponent(crusta);

    //connect probe tools
    SurfaceProbeTool* probe = dynamic_cast<SurfaceProbeTool*>(cbData->tool);
    if (probe != NULL)
    {
        probe->getSampleCallbacks().add(
            this, &CrustaApp::surfaceSamplePickedCallback);
    }

#if CRUSTA_ENABLE_DEBUG
    //check for the creation of the debug tool
    DebugTool* dbgTool = dynamic_cast<DebugTool*>(cbData->tool);
    if (dbgTool!=NULL && crusta->debugTool==NULL)
        crusta->debugTool = dbgTool;
#endif //CRUSTA_ENABLE_DEBUG


    Vrui::Application::toolCreationCallback(cbData);
}

void CrustaApp::
toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData)
{
    SurfaceProjector* surface = dynamic_cast<SurfaceProjector*>(cbData->tool);
    if (surface != NULL)
        PROJECTION_FAILED = false;

#if CRUSTA_ENABLE_DEBUG
    if (cbData->tool == crusta->debugTool)
        crusta->debugTool = NULL;
#endif //CRUSTA_ENABLE_DEBUG
}



END_CRUSTA


int main(int argc, char* argv[])
{
    char** appDefaults = 0; // This is an additional parameter no one ever uses
    crusta::CrustaApp app(argc, argv, appDefaults);

    app.run();

    return 0;
}
