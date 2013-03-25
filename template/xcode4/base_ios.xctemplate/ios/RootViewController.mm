#import "RootViewController.h"

int Portrait = PORTRAIT;

@implementation RootViewController

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
if( !Portrait )
    return UIInterfaceOrientationIsLandscape(interfaceOrientation);
else
    return UIInterfaceOrientationIsPortrait(interfaceOrientation);
}

- (NSUInteger) supportedInterfaceOrientations {
#ifdef __IPHONE_6_0
if( !Portrait )
    return UIInterfaceOrientationMaskLandscape;
else
    return UIInterfaceOrientationMaskPortrait;
#endif
}

- (BOOL) shouldAutorotate {
    return YES;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)viewDidUnload {
    [super viewDidUnload];
}

- (void)dealloc {
    [super dealloc];
}

@end
