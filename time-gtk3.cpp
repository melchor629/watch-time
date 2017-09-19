//g++ --std=c++11 `pkg-config --cflags gtk+-3.0` -Os -o time time-gtk3.cpp `pkg-config --libs gtk+-3.0` && ./time
#include <gtk/gtk.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <sys/stat.h>
#include <signal.h>

using namespace std;

struct TimeAppConfig {
    struct {
        uint16_t red = 0, green = 0, blue = 0;
    } color;
    struct {
        double red = 1.0, green = 1.0, blue = 1.0;
    } color2;
    string fontName = "Ubuntu Mono";
    uint32_t fontSize = 70 * PANGO_SCALE;
    PangoStyle fontStyle = PANGO_STYLE_NORMAL;
    PangoWeight fontWeight = PANGO_WEIGHT_NORMAL;
    bool analogicoVisible = false;
    struct {
        int32_t x = 30, y = 30;
    } position;
};

struct TimeApp {
    GtkApplication* app;
    GtkWidget* window;
    GtkWidget* cont;
    GtkWidget* tiempo;
    GtkWidget* secundero;
    GtkWidget* analogico;
    bool analogicoVisible = false;
    PangoAttrList* attr;
    TimeAppConfig config;
};

static void screenChanged(GtkWidget* widget, GdkScreen* old, gpointer data);
static bool draw(GtkWidget* widget, cairo_t* new_cr, gpointer data);
static void clicked(GtkWindow* window, GdkEventButton* event, gpointer data);
static bool a(TimeApp* self);
static bool dibujarAnalogico(GtkWidget* widget, cairo_t* cr, TimeApp* data);
static void cambiarModo(TimeApp* data); //Ctrl+S
static void cambiarColor(TimeApp* data); //Ctrl+C
static void cambiarFuente(TimeApp* data); //Ctrl+F
static void salir(TimeApp* data); //Ctrl+Q
static void leerConfiguracion(TimeApp* self);
static void guardarConfiguracion(TimeApp* self);
static void saliendo(GtkWidget*, GdkEvent*, TimeApp* self);
static void sigint(int, siginfo_t* info, void*);

//https://stackoverflow.com/questions/3908565/how-to-make-gtk-window-background-transparent
static void transparente(GtkWidget* window) {
    gtk_widget_set_app_paintable(window, true);
    g_signal_connect(G_OBJECT(window), "draw", G_CALLBACK(draw), NULL);
    g_signal_connect(G_OBJECT(window), "screen-changed", G_CALLBACK(screenChanged), NULL);
    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(clicked), NULL);
}

static void resizeWindow(TimeApp* self) {
    PangoAttrIterator* it = pango_attr_list_get_iterator(self->attr);
    PangoAttrInt* fontSize = (PangoAttrInt*) pango_attr_iterator_get(it, PANGO_ATTR_SIZE);
    double size = fontSize->value / PANGO_SCALE;
    pango_attr_iterator_destroy(it);

    gtk_widget_set_size_request(self->window, 4, 1);
    gtk_window_resize(GTK_WINDOW(self->window), 400.0 / 70.0 * size, 100.0 / 70.0 * size);
}

static void activate(GtkApplication* app, gpointer data) {
    TimeApp* self = (TimeApp*) data;
    self->analogicoVisible = !self->config.analogicoVisible;
    self->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(self->window), "Watch time");
    gtk_window_set_default_size(GTK_WINDOW(self->window), 400, 100);
    gtk_window_set_decorated(GTK_WINDOW(self->window), false);
    gtk_window_set_resizable(GTK_WINDOW(self->window), false);
    transparente(self->window);
    g_signal_connect(self->window, "delete-event", G_CALLBACK(saliendo), self);

    self->cont = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(self->window), self->cont);

    self->attr = pango_attr_list_new();
    pango_attr_list_insert(self->attr, pango_attr_family_new(self->config.fontName.c_str()));
    pango_attr_list_insert(self->attr, pango_attr_style_new(self->config.fontStyle));
    pango_attr_list_insert(self->attr, pango_attr_weight_new(self->config.fontWeight));
    pango_attr_list_insert(self->attr, pango_attr_size_new(self->config.fontSize));
    pango_attr_list_insert(self->attr, pango_attr_foreground_new(
        self->config.color.red,
        self->config.color.green,
        self->config.color.blue
    ));

    self->analogico = gtk_drawing_area_new();
    gtk_widget_set_size_request(self->analogico, 100, 100);
    g_signal_connect(G_OBJECT(self->analogico), "draw", G_CALLBACK(dibujarAnalogico), self);

    self->tiempo = gtk_label_new("        ");
    self->secundero = gtk_label_new("  :  :  ");
    gtk_label_set_attributes(GTK_LABEL(self->tiempo), self->attr);
    gtk_label_set_attributes(GTK_LABEL(self->secundero), self->attr);

    gtk_fixed_put(GTK_FIXED(self->cont), self->analogico, 0, 0);
    gtk_fixed_put(GTK_FIXED(self->cont), self->tiempo, 0, 0);
    gtk_fixed_put(GTK_FIXED(self->cont), self->secundero, 0, 0);

    resizeWindow(self);
    screenChanged(self->window, NULL, NULL);
    gtk_widget_show_all(self->window);
    cambiarModo(self);
    gtk_window_move(GTK_WINDOW(self->window), self->config.position.x, self->config.position.y);

    a(self);
    g_timeout_add(100, (GSourceFunc) a, self);

    GtkAccelGroup* gag = gtk_accel_group_new();
    gtk_accel_group_connect(
        gag,
        gdk_keyval_from_name("s"),
        GDK_CONTROL_MASK,
        GtkAccelFlags(0),
        g_cclosure_new_swap(GCallback(cambiarModo), self, NULL)
    );
    gtk_accel_group_connect(
        gag,
        gdk_keyval_from_name("c"),
        GDK_CONTROL_MASK,
        GtkAccelFlags(0),
        g_cclosure_new_swap(GCallback(cambiarColor), self, NULL)
    );
    gtk_accel_group_connect(
        gag,
        gdk_keyval_from_name("f"),
        GDK_CONTROL_MASK,
        GtkAccelFlags(0),
        g_cclosure_new_swap(GCallback(cambiarFuente), self, NULL)
    );
    gtk_accel_group_connect(
        gag,
        gdk_keyval_from_name("q"),
        GDK_CONTROL_MASK,
        GtkAccelFlags(0),
        g_cclosure_new_swap(GCallback(salir), self, NULL)
    );
    gtk_window_add_accel_group(GTK_WINDOW(self->window), gag);
}

static TimeApp* leapp;
int main(int argc, char* argv[]) {
    TimeApp ledata;
    leapp = &ledata;
    leerConfiguracion(&ledata);

    struct sigaction sig = {0};
    sig.sa_sigaction = sigint;
    sig.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sig, NULL);

    #define RESET "\x1b[0m"
    #define W "\x1b[31m"
    printf(
        "Keybard combinations:\n"
        "    - Digital clock watch:\n"
        "        " W "(Ctrl|Meta)+C" RESET "  Changes the text color\n"
        "        " W "(Ctrl|Meta)+F" RESET "  Changes the font (it is recommended fixed width fonts)\n"
        "    - Analog clock mode:\n"
        "        " W "(Ctrl|Meta)+C" RESET "  Changes the watch color\n"
        "    " W "(Ctrl|Meta)+S" RESET "      Cambia entre los dos modos\n"
        "    " W "Ctrl+Q" RESET "             Closes the watch\n"
        "    " W "Alt+Click" RESET "          Moves the window\n\n"
        "All changes you make will be saved instantaninally :)\n"
        "Enjoy - melchor9000\n"
    );

    GtkApplication* app = gtk_application_new("me.melchor9000.time", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &ledata);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}



static bool supportsAlpha = false;
static void screenChanged(GtkWidget* widget, GdkScreen* old, gpointer data) {
    GdkScreen* screen = gtk_widget_get_screen(widget);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
    if(!visual) {
        fprintf(stderr, "Your screen doesn't support alpha!\n");
        fprintf(stderr, "Maybe you lack a compositor or the compositor doesn't suppor transparencies\n");
        visual = gdk_screen_get_system_visual(screen);
    } else {
        supportsAlpha = true;
    }
    gtk_widget_set_visual(widget, visual);
}

static bool draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    cairo_t* newCr = gdk_cairo_create(gtk_widget_get_window(widget));
    if(supportsAlpha) {
        cairo_set_source_rgba(newCr, 0, 0, 0, 0);
    } else {
        cairo_set_source_rgb(newCr, 0, 0, 0);
    }
    cairo_set_operator(newCr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(newCr);
    cairo_destroy(newCr);
    return false;
}

static void clicked(GtkWindow* window, GdkEventButton* event, gpointer data) {
    gtk_window_set_decorated(window, !gtk_window_get_decorated(window));
}

static bool a(TimeApp* self) {
    if(self->analogicoVisible) {
        gtk_widget_queue_draw_area(self->cont, 0, 0, 0, 0);
        gdk_window_invalidate_rect(gtk_widget_get_window(self->window), NULL, false);
    } else {
        timespec ts;
        time_t t = time(NULL);
        struct tm lt = {0};
        localtime_r(&t, &lt);
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned h, m, s;
        unsigned long tvsec = ts.tv_sec + lt.tm_gmtoff;

        s = tvsec % 60;
        m = (tvsec / 60) % 60;
        h = (tvsec / 3600) % 24;

        char tmpStr[9];
        snprintf(tmpStr, 9, "%.2u %.2u %.2u", h, m, s);
        gtk_label_set_label(GTK_LABEL(self->tiempo), tmpStr);
        if(s % 2) {
            gtk_widget_set_visible(self->secundero, false);
        } else {
            gtk_widget_set_visible(self->secundero, true);
        }
    }

    return true;
}


#define fdiv(a,b) ((float) a / (float) b)
struct NSPoint { double x, y; };
NSPoint operator+(const NSPoint &a, const NSPoint &b) { return NSPoint{a.x + b.x, a.y + b.y}; };
#define NSMakePoint(x, y) (NSPoint{x,y})
static inline NSPoint timeToPoint(unsigned time, unsigned max, float amp, float extra = 0.0f) {
    float angle = 2 * 3.1415926536f * (fdiv(time, max) + 1.f / max * extra) - 3.1415926536/2;
    return NSMakePoint(
        amp * cos(angle),
        amp * sin(angle)
    );
}

static bool dibujarAnalogico(GtkWidget* widget, cairo_t* cr, TimeApp* self) {
    if(self->analogicoVisible) {
        timespec ts;
        time_t t = time(NULL);
        struct tm lt = {0};
        localtime_r(&t, &lt);
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned h, m, s;
        unsigned long tvsec = ts.tv_sec + lt.tm_gmtoff;

        s = tvsec % 60;
        m = (tvsec / 60) % 60;
        h = (tvsec / 3600) % 24;

        const double width = gtk_widget_get_allocated_width(widget);
        const double height = gtk_widget_get_allocated_height(widget);
        const double lineWidth = 5;
        const double radius = min(width, height) / 2 - lineWidth;
        const NSPoint center = NSMakePoint(width / 2, height / 2);

        cairo_set_line_width(cr, lineWidth);
        cairo_set_source_rgb(cr, self->config.color2.red, self->config.color2.green, self->config.color2.blue);
        cairo_translate(cr, width / 2, height / 2);
        cairo_arc(cr, 0, 0, radius, 0, 2 * 3.1415926536);
        cairo_stroke(cr);
        cairo_identity_matrix(cr);

        cairo_move_to(cr, center.x, center.y);
        NSPoint point = timeToPoint(h % 12, 12, height / 5, fdiv(m, 60)) + center;
        cairo_line_to(cr, point.x, point.y);
        cairo_stroke(cr);

        cairo_move_to(cr, center.x, center.y);
        point = timeToPoint(m, 60, height / 4, fdiv(s, 60)) + center;
        cairo_line_to(cr, point.x, point.y);
        cairo_stroke(cr);

        cairo_move_to(cr, center.x, center.y);
        point = timeToPoint(s, 60, height / 3, fdiv(ts.tv_nsec, 1000000000)) + center;
        cairo_line_to(cr, point.x, point.y);
        cairo_stroke(cr);
    }

    return false;
}

static void cambiarModo(TimeApp* self) {
    self->analogicoVisible = self->config.analogicoVisible = !self->analogicoVisible;
    if(self->analogicoVisible) {
        gtk_widget_set_visible(self->analogico, true);
        gtk_widget_set_visible(self->secundero, false);
        gtk_widget_set_visible(self->tiempo, false);
    } else {
        gtk_widget_set_visible(self->analogico, false);
        gtk_widget_set_visible(self->secundero, true);
        gtk_widget_set_visible(self->tiempo, true);
    }
    guardarConfiguracion(self);
}

static void aplicarCambioColor(GtkColorChooser* dialog, gint r, TimeApp* self) {
    if(r != GTK_RESPONSE_CANCEL && r != GTK_RESPONSE_DELETE_EVENT) {
        GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &color);
        if(!self->analogicoVisible) {
            pango_attr_list_change(self->attr, pango_attr_foreground_new(
                self->config.color.red = color.red * 65536.0,
                self->config.color.green = color.green * 65536.0,
                self->config.color.blue = color.blue * 65536.0
            ));
            gtk_label_set_attributes(GTK_LABEL(self->tiempo), self->attr);
            gtk_label_set_attributes(GTK_LABEL(self->secundero), self->attr);
        } else {
            self->config.color2.red = color.red;
            self->config.color2.green = color.green;
            self->config.color2.blue = color.blue;
        }
        guardarConfiguracion(self);
    } else {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

static void cambiarColor(TimeApp* self) {
    GtkWidget* dialog = gtk_color_chooser_dialog_new("Watch color", GTK_WINDOW(self->window));
    if(!self->analogicoVisible) {
        PangoAttrIterator* it = pango_attr_list_get_iterator(self->attr);
        PangoAttrColor* attr = (PangoAttrColor*) pango_attr_iterator_get(it, PANGO_ATTR_FOREGROUND);
        GdkRGBA color {
            double(attr->color.red) / 65536.0,
            double(attr->color.green) / 65536.0,
            double(attr->color.blue) / 65536.0,
            1
        };
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dialog), &color);
        gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(dialog), false);
        pango_attr_iterator_destroy(it);
    } else {
        GdkRGBA color {
            self->config.color2.red,
            self->config.color2.green,
            self->config.color2.blue,
            1
        };
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dialog), &color);
        gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(dialog), false);
    }

    g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(aplicarCambioColor), self);
    gtk_widget_show_all(dialog);
}

static void aplicarCambioFuente(GtkFontChooser* dialog, gint r, TimeApp* self) {
    if(r != GTK_RESPONSE_CANCEL && r != GTK_RESPONSE_DELETE_EVENT) {
        PangoFontDescription* font = gtk_font_chooser_get_font_desc(dialog);
        pango_attr_list_change(self->attr, pango_attr_family_new(
            (self->config.fontName = pango_font_description_get_family(font)).c_str()
        ));
        pango_attr_list_change(self->attr, pango_attr_style_new(
            self->config.fontStyle = pango_font_description_get_style(font)
        ));
        pango_attr_list_change(self->attr, pango_attr_weight_new(
            self->config.fontWeight = pango_font_description_get_weight(font)
        ));
        pango_attr_list_change(self->attr, pango_attr_size_new(
            self->config.fontSize = pango_font_description_get_size(font)
        ));
        gtk_label_set_attributes(GTK_LABEL(self->tiempo), self->attr);
        gtk_label_set_attributes(GTK_LABEL(self->secundero), self->attr);
        resizeWindow(self);
        guardarConfiguracion(self);
    } else {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

static void cambiarFuente(TimeApp* self) {
    if(!self->analogicoVisible) {
        PangoAttrIterator* it = pango_attr_list_get_iterator(self->attr);
        PangoAttrString* fontName = (PangoAttrString*) pango_attr_iterator_get(it, PANGO_ATTR_FAMILY);
        PangoAttrInt* fontStyle = (PangoAttrInt*) pango_attr_iterator_get(it, PANGO_ATTR_STYLE);
        PangoAttrInt* fontWeight = (PangoAttrInt*) pango_attr_iterator_get(it, PANGO_ATTR_WEIGHT);
        PangoAttrInt* fontSize = (PangoAttrInt*) pango_attr_iterator_get(it, PANGO_ATTR_SIZE);
        PangoFontDescription* font = pango_font_description_new();
        pango_font_description_set_family(font, fontName->value);
        pango_font_description_set_style(font, PangoStyle(fontStyle->value));
        pango_font_description_set_weight(font, PangoWeight(fontWeight->value));
        pango_font_description_set_size(font, fontSize->value);
        GtkWidget* dialog = gtk_font_chooser_dialog_new("Watch font", GTK_WINDOW(self->window));
        gtk_font_chooser_set_font_desc(GTK_FONT_CHOOSER(dialog), font);
        gtk_font_chooser_set_preview_text(GTK_FONT_CHOOSER(dialog), "09:11:00");
        pango_attr_iterator_destroy(it);
        pango_font_description_free(font);

        g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(aplicarCambioFuente), self);
        gtk_widget_show_all(dialog);
    }
}

static void salir(TimeApp* self) {
    gtk_window_close(GTK_WINDOW(self->window));
}

static void leerConfiguracion(TimeApp* self) {
    string path = getenv("HOME");
    path += "/.config/time-gtk3.conf";
    struct stat info;
    if(!stat(path.c_str(), &info)) {
#define fread_(var) fread(&(var), sizeof(var), 1, file);
        FILE* file = fopen(path.c_str(), "r");
        fread_(self->config.color.red);
        fread_(self->config.color.green);
        fread_(self->config.color.blue);
        fread_(self->config.color2.red);
        fread_(self->config.color2.green);
        fread_(self->config.color2.blue);
        size_t sizeOfString = self->config.fontName.size();
        fread_(sizeOfString);
        char str[sizeOfString];
        fread(str, sizeof(char), sizeOfString, file);
        self->config.fontName = string(str, sizeOfString);
        fread_(self->config.fontSize);
        fread_(self->config.fontWeight);
        fread_(self->config.fontStyle);
        fread_(self->config.analogicoVisible);
        fread_(self->config.position.x);
        fread_(self->config.position.y);
#undef fread_
    } else {
        guardarConfiguracion(self);
    }
}

static void guardarConfiguracion(TimeApp* self) {
    string path = getenv("HOME");
    path += "/.config/time-gtk3.conf";
#define fwrite_(var) fwrite(&(var), sizeof(var), 1, file);
    FILE* file = fopen(path.c_str(), "w");
    fwrite_(self->config.color.red);
    fwrite_(self->config.color.green);
    fwrite_(self->config.color.blue);
    fwrite_(self->config.color2.red);
    fwrite_(self->config.color2.green);
    fwrite_(self->config.color2.blue);
    size_t sizeOfString = self->config.fontName.size();
    fwrite_(sizeOfString);
    fwrite(self->config.fontName.c_str(), sizeof(char), sizeOfString, file);
    fwrite_(self->config.fontSize);
    fwrite_(self->config.fontWeight);
    fwrite_(self->config.fontStyle);
    fwrite_(self->config.analogicoVisible);
    fwrite_(self->config.position.x);
    fwrite_(self->config.position.y);
    fclose(file);
#undef fwrite_
}

static void saliendo(GtkWidget*, GdkEvent*, TimeApp* self) {
    gtk_window_get_position(GTK_WINDOW(self->window), &(self->config.position.x), &(self->config.position.y));
    guardarConfiguracion(self);
}

static void sigint(int, siginfo_t*, void*) {
    gtk_window_close(GTK_WINDOW(leapp->window));
}