#import <UIKit/UIKit.h>
#import <Foundation/NSBundle.h>
#import "AppController.h"
#import "RootViewController.h"

#include <string>
#include "bsgl.h"

@implementation AppController

@synthesize window;
@synthesize viewController;
@synthesize glView;

#pragma mark -
#pragma mark Application lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    glView = [[[OpenGLView alloc] initWithFrame:screenBounds] autorelease];

    viewController = [[RootViewController alloc] initWithNibName:nil bundle:nil];
    viewController.wantsFullScreenLayout = YES;
    viewController.view = glView;

    if( [[UIDevice currentDevice].systemVersion floatValue] < 6.0) {
        // warning: addSubView doesn't work on iOS6
        [window addSubview:viewController.view];
    }else {
        // use this method on ios6
        [window setRootViewController:viewController];
    }
    
    [window makeKeyAndVisible];

    [[UIApplication sharedApplication] setStatusBarHidden: YES];

    bsglCreate(BSGL_VERSION)->System_Start();

    return YES;
}


- (void)applicationWillResignActive:(UIApplication*)application {
}

- (void)applicationDidBecomeActive:(UIApplication*)application {
}

- (void)applicationDidEnterBackground:(UIApplication*)application {
}

- (void)applicationWillEnterForeground:(UIApplication*)application {
}

- (void)applicationWillTerminate:(UIApplication*)application {
}


#pragma mark -
#pragma mark Memory management

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application {
}


- (void)dealloc {
    [super dealloc];
}

@end

std::string mmGetFullPath(char const* filename) {
    NSString* fn = [NSString stringWithCString:filename encoding:NSASCIIStringEncoding];
    NSString* path = [[NSBundle mainBundle] pathForResource:[[fn lastPathComponent] stringByDeletingPathExtension] ofType:[fn pathExtension]];
    return std::string([path UTF8String]);
}
