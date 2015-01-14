#import "OpenGLView.h"
#import <Foundation/NSTimer.h>

extern void bsgl_begin();
extern void bsgl_end();

typedef bool (*FrameFunc)();
FrameFunc logic;
FrameFunc render;

OpenGLView* v;

@implementation OpenGLView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (void)setupLayer {
    _eaglLayer = (CAEAGLLayer*)self.layer;
    _eaglLayer.opaque = YES;
}

- (void)setupContext {
    EAGLRenderingAPI api = kEAGLRenderingAPIOpenGLES1;
    _context = [[EAGLContext alloc] initWithAPI:api];

    if (!_context) {
        NSLog(@"Failed to initialize OpenGLES context");
        exit(1);
    }
    
    if (![EAGLContext setCurrentContext:_context]) {
        NSLog(@"Failed to set current OpenGL context");
        exit(1);
    }
}

- (void)setupRenderBuffer {
    glGenRenderbuffersOES(1, &_colorRenderBuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, _colorRenderBuffer);
    [_context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:_eaglLayer];
}

- (void)setupFrameBuffer {
    GLuint framebuffer;
    glGenFramebuffersOES(1, &framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                              GL_RENDERBUFFER_OES, _colorRenderBuffer);
}

- (void)logicFunc:(NSTimer*)timer {
    logic();
}

- (void)renderFunc:(NSTimer*)timer {
    render();
    [_context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

- (void)setupLogicFunc:(FrameFunc)func withFPS:(int)fps {
    logic = func;
    [NSTimer scheduledTimerWithTimeInterval:1.0f/fps
                                     target:self
                                   selector:@selector(logicFunc:)
                                   userInfo:nil
                                    repeats:YES];
}

- (void)setupRenderFunc:(FrameFunc)func withFPS:(int)fps withDeltaTime:(float*)dt {
    render = func;
    *dt = 1.0f/fps;
    [NSTimer scheduledTimerWithTimeInterval:1.0f/fps
                                     target:self
                                   selector:@selector(renderFunc:)
                                   userInfo:nil
                                    repeats:YES];
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.contentScaleFactor = 2.0f;
        [self setupLayer];
        [self setupContext];
        [self setupRenderBuffer];
        [self setupFrameBuffer];

        v = self;        
        bsgl_begin();
    }
    return self;
}

- (int)getWidth {
	CGSize bound = [self bounds].size;
	return bound.width;
}

- (int)getHeight {
	CGSize bound = [self bounds].size;
	return bound.height;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
    for (UITouch* touch in touches) {
    }
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event {
    for (UITouch* touch in touches) {
    }
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
    for (UITouch* touch in touches) {
    }
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
    for (UITouch* touch in touches) {
    }
}

- (void)dealloc {
    bsgl_end();
    [_context release];
    _context = nil;
    [super dealloc];
}

@end

void mmSetLogic(FrameFunc func, int fps) {
    [v setupLogicFunc:func withFPS:fps];
}

void mmSetRender(FrameFunc func, int fps, float* dt) {
    [v setupRenderFunc:func withFPS:fps withDeltaTime:dt];
}

float mmGetViewContentScaleFactor() {
    return v.contentScaleFactor;
}

int mmGetViewWidth() {
    return [v getWidth];
}

int mmGetViewHeight() {
    return [v getHeight];
}
