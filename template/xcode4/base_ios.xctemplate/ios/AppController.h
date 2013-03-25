#import "OpenGLView.h"

@class RootViewController;

@interface AppController : NSObject <UIAccelerometerDelegate, UIAlertViewDelegate, UITextFieldDelegate, UIApplicationDelegate> {
    UIWindow*               window;
    RootViewController*     viewController;
    OpenGLView*             glView;
}

@property (nonatomic, retain) UIWindow* window;
@property (nonatomic, retain) RootViewController* viewController;
@property (nonatomic, retain) OpenGLView* glView;

@end

