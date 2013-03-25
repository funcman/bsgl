#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

@interface OpenGLView : UIView {
    CAEAGLLayer*    _eaglLayer;
    EAGLContext*    _context;
    GLuint          _colorRenderBuffer;
}

@end
