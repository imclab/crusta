#ifndef _Crusta_SurfaceProjector_H_
#define _Crusta_SurfaceProjector_H_


#include <GL/GLContextData.h>
#include <Vrui/InputDevice.h>

#include <crusta/CrustaComponent.h>
#include <crusta/SurfacePoint.h>


namespace crusta {


class SurfaceProjector : public CrustaComponent
{
public:
    SurfaceProjector();

    SurfacePoint project(Vrui::InputDevice* device, bool mapBackToDevice=true);

    void display(GLContextData& contextData,
                 const Vrui::NavTransform& original,
                 const Vrui::NavTransform& transform) const;

protected:
    bool projectionFailed;
};


} //namespace crusta


#endif //_Crusta_SurfaceProjector_H_
