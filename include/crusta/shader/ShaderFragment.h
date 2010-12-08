#ifndef _ShaderFragment_H_
#define _ShaderFragment_H_


#include <algorithm>
#include <vector>

#include <crusta/basics.h>
#include <crusta/QuadNodeData.h> // for SubRegion


BEGIN_CRUSTA


///\todo comment!!
class ShaderFragment
{
public:
    ShaderFragment();
    virtual ~ShaderFragment();

    virtual void reset();
    virtual void initUniforms(GLuint programObj) = 0;
    virtual bool update() = 0;
    virtual std::string getCode() = 0;

protected:
    bool codeEmitted;

    std::string makeUniqueName(const std::string& baseName) const;

private:
///\todo use IdGenerator?
    static int nextSequenceNumber; // to generate unique uniform names
    int sequenceNumber;
};


END_CRUSTA


#endif //_ShaderFragment_H_
