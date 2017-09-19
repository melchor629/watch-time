//clang time.mm -framework Cocoa -framework Foundation -Os -o time -mmacosx-version-min=10.6 -fobjc-arc -lc++ && ./time
#import <Cocoa/Cocoa.h>
#include <cmath>
#include <ctime>

void a(void* ao);

@interface TimeApp: NSObject

    @property (readonly,nonatomic,retain) NSWindow* win;
    @property (readonly,nonatomic,retain) NSView* cont;
    @property (readonly,nonatomic,retain) NSTextField* tiempo;
    @property (readonly,nonatomic,retain) NSTextField* secundero;
    @property (nonatomic,retain) NSColor* color;
    @property (readonly,nonatomic,retain) NSFont* font;

    - (id) init;
    - (IBAction) siempreEncima: (NSMenuItem*) item;
    - (IBAction) cambiarColor;
    - (void) aplicarCambioColor: (NSColorPanel*) color;
    - (IBAction) cambiarFuente;
    - (void) aplicarCambioFuente;
    - (IBAction) ocultar: (NSMenuItem*) item;
    - (IBAction) salir;

@end

@implementation TimeApp: NSObject

    - (void) crearVista {
        _cont = [[NSView alloc] initWithFrame: NSMakeRect(0, 0, 400, 100)];
        _tiempo = [[NSTextField alloc] init];
        _secundero = [[NSTextField alloc] init];
        [_tiempo setStringValue: @"        "];
        [_secundero setStringValue: @"  :  :  "];
        [_tiempo setFont: _font];
        [_secundero setFont: _font];
        [_tiempo setTextColor: _color];
        [_secundero setTextColor: _color];
        [_tiempo setFrame: [_cont frame]];
        [_secundero setFrame: [_cont frame]];
        [_tiempo setEditable: false];
        [_secundero setEditable: false];
        [_tiempo setSelectable: false];
        [_secundero setSelectable: false];
        [_tiempo setBezeled: false];
        [_secundero setBezeled: false];
        [_tiempo setDrawsBackground: false];
        [_secundero setDrawsBackground: false];

        [_cont addSubview: _tiempo];
        [_cont addSubview: _secundero];
        [_win setContentView: _cont];
        [_win cascadeTopLeftFromPoint: NSMakePoint(20, 20)];
        [_win makeKeyAndOrderFront: nil];
        [_win setTitle: @"Reloj"];
    }

    - (void) crearMenu {
        NSMenu* menu = [[NSMenu alloc] initWithTitle: @"time"];
        NSMenu* submenu = [[NSMenu alloc] initWithTitle: @"time"];
        NSMenuItem* item1 = [[NSMenuItem alloc] initWithTitle: @"ignored" action: nil keyEquivalent: @""];
        NSMenuItem* subitem1 = [[NSMenuItem alloc] initWithTitle: @"Always on top" action: @selector(siempreEncima:) keyEquivalent: @"s"];
        NSMenuItem* subitem2 = [[NSMenuItem alloc] initWithTitle: @"Hide window" action: @selector(ocultar:) keyEquivalent: @"h"];
        NSMenuItem* subitem3 = [[NSMenuItem alloc] initWithTitle: @"Change color" action: @selector(cambiarColor) keyEquivalent: @"c"];
        NSMenuItem* subitem4 = [[NSMenuItem alloc] initWithTitle: @"Change font" action: @selector(cambiarFuente) keyEquivalent: @"f"];
        NSMenuItem* subitem5 = [[NSMenuItem alloc] initWithTitle: @"Quit" action: @selector(salir) keyEquivalent: @"q"];

        [subitem1 setTarget: self];
        [subitem2 setTarget: self];
        [subitem3 setTarget: self];
        [subitem4 setTarget: self];
        [subitem5 setTarget: self];

        [submenu addItem: subitem1];
        [submenu addItem: subitem2];
        [submenu addItem: subitem3];
        [submenu addItem: subitem4];
        [submenu addItem: subitem5];
        [menu addItem: item1];
        [menu setSubmenu: submenu forItem: item1];

        [NSApp setMainMenu: menu];
    }

    - (id) init {
        self = [super init];
        _win = [[NSWindow alloc] initWithContentRect: NSMakeRect(0, 0, 400, 100)
                                          styleMask: NSClosableWindowMask
                                          backing: NSBackingStoreNonretained
                                          defer: NO];
        [_win setBackgroundColor: [NSColor colorWithRed: 0 green: 0 blue: 0 alpha: 0]];
        [_win setMovable: true];
        [_win setMovableByWindowBackground: true];

        _color = [NSColor colorWithRed: [[NSUserDefaults standardUserDefaults] doubleForKey: @"color:red"]
                                 green: [[NSUserDefaults standardUserDefaults] doubleForKey: @"color:green"]
                                  blue: [[NSUserDefaults standardUserDefaults] doubleForKey: @"color:blue"]
                                 alpha: 1.0];

        if([[NSUserDefaults standardUserDefaults] objectForKey: @"font:name"] != nil) {
            _font = [NSFont fontWithName: [[NSUserDefaults standardUserDefaults] objectForKey: @"font:name"] size: [[NSUserDefaults standardUserDefaults] doubleForKey: @"font:size"]];
        } else {
            _font = [NSFont fontWithName: @"Courier New" size: 70];
        }

        [self crearVista];
        [self crearMenu];
        [self resize: [_font pointSize]];

        NSRect windowFrame = [_win frame];
        windowFrame.origin.x = [[NSUserDefaults standardUserDefaults] doubleForKey: @"window:x"];
        windowFrame.origin.y = [[NSUserDefaults standardUserDefaults] doubleForKey: @"window:y"];
        [_win setFrame: windowFrame display: true animate: false];

        dispatch_async_f(dispatch_get_main_queue(), (__bridge void*) self, a);
        return self;
    }

    - (IBAction) siempreEncima: (NSMenuItem*) item {
        if([item state] == NSOnState) {
            [item setState: NSOffState];
            [_win setLevel: NSNormalWindowLevel];
        } else {
            [item setState: NSOnState];
            [_win setLevel: NSStatusWindowLevel];
        }
    }

    - (IBAction) cambiarColor {
        [[NSColorPanel sharedColorPanel] setColor: _color];
        [[NSColorPanel sharedColorPanel] setTarget: self];
        [[NSColorPanel sharedColorPanel] setAction: @selector(aplicarCambioColor:)];
        [[NSColorPanel sharedColorPanel] setShowsAlpha: false];
        [[NSColorPanel sharedColorPanel] makeKeyAndOrderFront: nil];
    }

    - (IBAction) salir {
        [[NSUserDefaults standardUserDefaults] setDouble: [_win frame].origin.x forKey: @"window:x"];
        [[NSUserDefaults standardUserDefaults] setDouble: [_win frame].origin.y forKey: @"window:y"];
        [NSApp terminate: nil];
    }

    - (void) aplicarCambioColor: (NSColorPanel*) color; {
        _color = [color color];
        [_tiempo setTextColor: _color];
        [_secundero setTextColor: _color];
        [[NSUserDefaults standardUserDefaults] setDouble: [_color redComponent] forKey: @"color:red"];
        [[NSUserDefaults standardUserDefaults] setDouble: [_color greenComponent] forKey: @"color:green"];
        [[NSUserDefaults standardUserDefaults] setDouble: [_color blueComponent] forKey: @"color:blue"];
    }

    - (IBAction) cambiarFuente {
        NSFontManager* s = [NSFontManager sharedFontManager];
        [s setTarget: self];
        [s setAction: @selector(aplicarCambioFuente)];
        [s setSelectedFont: _font isMultiple: false];
        [[s fontPanel: true] makeKeyAndOrderFront: nil];
    }

    - (void) aplicarCambioFuente {
        _font = [[NSFontManager sharedFontManager] convertFont:[[NSFontManager sharedFontManager] selectedFont]];
        [_tiempo setFont: _font];
        [_secundero setFont: _font];
        [[NSUserDefaults standardUserDefaults] setObject: [_font fontName] forKey: @"font:name"];
        [[NSUserDefaults standardUserDefaults] setDouble: [_font pointSize] forKey: @"font:size"];
        [self resize: [_font pointSize]];
        [_win invalidateShadow];
    }

    - (void) resize: (float) pointSize {
        [_win setContentSize: NSMakeSize(400.0 / 70.0 * pointSize, 100.0 / 70.0 * pointSize)];
        [_cont setFrameSize: NSMakeSize(400.0 / 70.0 * pointSize, 100.0 / 70.0 * pointSize)];
        [_tiempo setFrame: [_cont frame]];
        [_secundero setFrame: [_cont frame]];
    }

    - (IBAction) ocultar: (NSMenuItem*) item {
        if([item state] == NSOnState) {
            [item setState: NSOffState];
            [_win orderBack: nil];
        } else {
            [item setState: NSOnState];
            [_win orderOut: nil];
        }
    }

@end

#define fdiv(a,b) ((float) a / (float) b)
static inline NSPoint timeToPoint(unsigned time, unsigned max, float amp, float extra = 0.0f) {
    float angle = 2 * 3.1415926536f * (fdiv(time, max) + 1.f / max * extra);
    return NSMakePoint(
        amp * sin(angle) + 256,
        amp * cos(angle) + 256
    );
}

static inline NSBezierPath* pathForTime(unsigned time, unsigned max, float amp, float extra = 0.0f) {
    NSBezierPath* path1 = [NSBezierPath bezierPath];
    [path1 moveToPoint: NSMakePoint(256, 256)];
    [path1 lineToPoint: timeToPoint(time, max, amp, extra)];
    [path1 closePath];
    return path1;
}

void a(void* ao) {
    TimeApp* e = (__bridge TimeApp*) ao;
    timespec ts;
    time_t t = time(NULL);
    struct tm lt = {0};
    localtime_r(&t, &lt);
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned h, m, s;
    unsigned long tvsec = ts.tv_sec + lt.tm_gmtoff;

    s = tvsec % 60;
    m = (tvsec / 60) % 60;
    h = (tvsec / 3600 + lt.tm_gmtoff) % 24;

    [[e tiempo] setStringValue: [NSString stringWithFormat:@"%.2u %.2u %.2u", h, m, s]];
    if(s % 2) {
        [e secundero].hidden = false;
    } else {
        [e secundero].hidden = true;
    }

    [[e win] invalidateShadow];


    {
        NSBezierPath.defaultLineWidth = 15.0;
        NSImage* dockImage = [[NSImage alloc] initWithSize: NSMakeSize(512, 512)];
        [dockImage lockFocus];
        [[NSColor whiteColor] setStroke];
        [[NSBezierPath bezierPathWithOvalInRect: NSMakeRect(58, 58, 400, 400)] stroke];
        [pathForTime(h % 12, 12, 100, fdiv(m, 60)) stroke];
        [pathForTime(m, 60, 150, fdiv(s, 60)) stroke];
        [pathForTime(s, 60, 200, fdiv(ts.tv_nsec, 1000000000)) stroke];
        [dockImage unlockFocus];
        [NSApp setApplicationIconImage: dockImage];
    }

    dispatch_time_t retraso = dispatch_time(DISPATCH_TIME_NOW, 100 * 1000000);
    dispatch_after_f(retraso, dispatch_get_main_queue(), ao, a);
}

// http://stackoverflow.com/questions/2926317/make-a-cocoa-application-quit-when-the-main-window-is-closed
@interface CloseableWindow : NSObject <NSApplicationDelegate>
- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) app;
@end

@implementation CloseableWindow : NSObject

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) app {
    return false;
}

@end

static TimeApp* app = nil;

static void sigint(int) {
    dispatch_async(dispatch_get_main_queue(), ^{
        [app salir];
    });
}

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
        app = [[TimeApp alloc] init];
        CloseableWindow* del = [[CloseableWindow alloc] init];
        signal(SIGINT, sigint);
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp setDelegate: del];
        [NSApp run];
    }
}
