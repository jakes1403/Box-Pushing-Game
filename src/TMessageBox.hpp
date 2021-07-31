// TMessagebox - Modified version of Boxer to be in single header
// See github project https://github.com/aaronmjacobs/Boxer

#ifndef TMESSAGEBOX_H
#define TMESSAGEBOX_H

namespace ThrusterNativeDialogs {

    /*!
     * Options for styles to apply to a message box
     */
    enum class Style {
        Info,
        Warning,
        Error,
        Question
    };

    /*!
     * Options for buttons to provide on a message box
     */
    enum class Buttons {
        OK,
        OKCancel,
        YesNo,
        Quit
    };

    /*!
     * Possible responses from a message box. 'None' signifies that no option was chosen, and 'Error' signifies that an
     * error was encountered while creating the message box.
     */
    enum class Selection {
        OK,
        Cancel,
        Yes,
        No,
        Quit,
        None,
        Error
    };

    Selection showMessageBox(const char* message, const char* title, Style style, Buttons buttons);

    /*!
     * The default style to apply to a message box
     */
    const Style kDefaultStyle = Style::Info;

    /*!
     * The default buttons to provide on a message box
     */
    const Buttons kDefaultButtons = Buttons::OK;
    /*!
     * Convenience function to call show() with the default buttons
     */
    inline Selection showMessageBox(const char* message, const char* title, Style style) {
        return showMessageBox(message, title, style, kDefaultButtons);
    }

    /*!
     * Convenience function to call show() with the default style
     */
    inline Selection showMessageBox(const char* message, const char* title, Buttons buttons) {
        return showMessageBox(message, title, kDefaultStyle, buttons);
    }

    /*!
     * Convenience function to call show() with the default style and buttons
     */
    inline Selection showMessageBox(const char* message, const char* title) {
        return showMessageBox(message, title, kDefaultStyle, kDefaultButtons);
    }

} // namespace boxer

#endif

#ifdef TMessageBox_WIN

#include <Windows.h>

namespace ThrusterNativeDialogs {

    namespace {

        UINT getIcon(Style style) {
            switch (style) {
            case Style::Info:
                return MB_ICONINFORMATION;
            case Style::Warning:
                return MB_ICONWARNING;
            case Style::Error:
                return MB_ICONERROR;
            case Style::Question:
                return MB_ICONQUESTION;
            default:
                return MB_ICONINFORMATION;
            }
        }

        UINT getButtons(Buttons buttons) {
            switch (buttons) {
            case Buttons::OK:
            case Buttons::Quit: // There is no 'Quit' button on Windows :(
                return MB_OK;
            case Buttons::OKCancel:
                return MB_OKCANCEL;
            case Buttons::YesNo:
                return MB_YESNO;
            default:
                return MB_OK;
            }
        }

        Selection getSelection(int response, Buttons buttons) {
            switch (response) {
            case IDOK:
                return buttons == Buttons::Quit ? Selection::Quit : Selection::OK;
            case IDCANCEL:
                return Selection::Cancel;
            case IDYES:
                return Selection::Yes;
            case IDNO:
                return Selection::No;
            default:
                return Selection::None;
            }
        }

    } // namespace

    Selection showMessageBox(const char* message, const char* title, Style style, Buttons buttons) {
        UINT flags = MB_TASKMODAL;

        flags |= getIcon(style);
        flags |= getButtons(buttons);

        return getSelection(MessageBoxA(nullptr, message, title, flags), buttons);
    }

} // namespace boxer

#endif

#ifdef TMessageBox_LIN

#include <gtk/gtk.h>

namespace ThrusterNativeDialogs {

    namespace {

        GtkMessageType getMessageType(Style style) {
            switch (style) {
            case Style::Info:
                return GTK_MESSAGE_INFO;
            case Style::Warning:
                return GTK_MESSAGE_WARNING;
            case Style::Error:
                return GTK_MESSAGE_ERROR;
            case Style::Question:
                return GTK_MESSAGE_QUESTION;
            default:
                return GTK_MESSAGE_INFO;
            }
        }

        GtkButtonsType getButtonsType(Buttons buttons) {
            switch (buttons) {
            case Buttons::OK:
                return GTK_BUTTONS_OK;
            case Buttons::OKCancel:
                return GTK_BUTTONS_OK_CANCEL;
            case Buttons::YesNo:
                return GTK_BUTTONS_YES_NO;
            case Buttons::Quit:
                return GTK_BUTTONS_CLOSE;
            default:
                return GTK_BUTTONS_OK;
            }
        }

        Selection getSelection(gint response) {
            switch (response) {
            case GTK_RESPONSE_OK:
                return Selection::OK;
            case GTK_RESPONSE_CANCEL:
                return Selection::Cancel;
            case GTK_RESPONSE_YES:
                return Selection::Yes;
            case GTK_RESPONSE_NO:
                return Selection::No;
            case GTK_RESPONSE_CLOSE:
                return Selection::Quit;
            default:
                return Selection::None;
            }
        }

    } // namespace

    Selection showMessageBox(const char* message, const char* title, Style style, Buttons buttons) {
        if (!gtk_init_check(0, nullptr)) {
            return Selection::Error;
        }

        // Create a parent window to stop gtk_dialog_run from complaining
        GtkWidget* parent = gtk_window_new(GTK_WINDOW_TOPLEVEL);

        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
            GTK_DIALOG_MODAL,
            getMessageType(style),
            getButtonsType(buttons),
            "%s",
            message);
        gtk_window_set_title(GTK_WINDOW(dialog), title);
        Selection selection = getSelection(gtk_dialog_run(GTK_DIALOG(dialog)));

        gtk_widget_destroy(GTK_WIDGET(dialog));
        gtk_widget_destroy(GTK_WIDGET(parent));
        while (g_main_context_iteration(nullptr, false));

        return selection;
    }

}

#endif