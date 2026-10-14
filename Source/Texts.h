#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
//  Все надписи интерфейса на двух языках.
//  В справке строка, начинающаяся с «# », — заголовок.
//==============================================================================
struct Texts
{
    juce::String lastMessage, in, out;
    juce::String point, pointHint;
    juce::String profile, noProfile, emptyFolder;
    juce::String saveCurrent, saveAs, import, exportCurve, folder, profilesPath;
    juce::String reset, help, close;
    juce::String saveAsTitle, profileName, ok, cancel, yes, no;
    juce::String replaceTitle, replaceMessage;
    juce::String errorTitle, readError, badJson, noPoints, badPoint, writeError;
    juce::String importTitle, exportTitle;
    juce::String jsonHelp;
    juce::String helpText;

    static juce::String u (const char* s) { return juce::String::fromUTF8 (s); }

    static Texts get (int language)
    {
        Texts t;

        if (language == 0)
        {
            t.lastMessage  = u ("Последнее сообщение");
            t.in           = u ("In");
            t.out          = u ("Out");
            t.point        = u ("Точка");
            t.pointHint    = u ("Выберите или поставьте точку");
            t.profile      = u ("Профиль");
            t.noProfile    = u ("— не выбран —");
            t.emptyFolder  = u ("В папке нет профилей");
            t.saveCurrent  = u ("Сохранить текущую кривую");
            t.saveAs       = u ("Сохранить как");
            t.import       = u ("Импорт");
            t.exportCurve  = u ("Экспорт");
            t.folder       = u ("Папка");
            t.profilesPath = u ("Профили лежат в папке:");
            t.reset        = u ("Сброс");
            t.help         = u ("Справка");
            t.close        = u ("Закрыть");
            t.saveAsTitle  = u ("Сохранить как");
            t.profileName  = u ("Название профиля:");
            t.ok           = u ("OK");
            t.cancel       = u ("Отмена");
            t.yes          = u ("Заменить");
            t.no           = u ("Отмена");
            t.replaceTitle = u ("Профиль уже есть");
            t.replaceMessage = u ("Профиль «%1» уже есть. Заменить его?");
            t.errorTitle   = u ("Ошибка");
            t.readError    = u ("Не удалось прочитать файл «%1».");
            t.badJson      = u ("Это не JSON или в нём ошибка: %1");
            t.noPoints     = u ("В файле нет списка точек \"points\".");
            t.badPoint     = u ("У точки №%1 нет \"x\" или \"y\".");
            t.writeError   = u ("Не удалось записать файл «%1».");
            t.importTitle  = u ("Импорт профиля");
            t.exportTitle  = u ("Экспорт кривой");
            t.jsonHelp     = u ("x — входящая velocity, y — исходящая, обе от 0 до 127. "
                                "curve — изгиб отрезка от этой точки до следующей: 0 — прямая, "
                                "больше 0 — выгнут вверх, меньше 0 — вниз, от -10 до 10. "
                                "Первая точка должна стоять на x = 0, последняя — на x = 127.");

            t.helpText = u (
                "# КАК РАБОТАЕТ\n"
                "MidiCurveVST меняет velocity нажатых нот по кривой. По горизонтали — velocity, "
                "которая пришла (In), по вертикали — velocity, которая уйдёт дальше (Out). "
                "Всё остальное (отпускания клавиш, CC, pitch bend и т.д.) проходит без изменений. "
                "Если кривая даёт 0, нота уходит с velocity 0.\n"
                "Чтобы ноты дошли до инструмента, выберите в MidiCurveVST MIDI output port, "
                "а в инструменте — такой же input port.\n"
                "\n"
                "# ТОЧКИ\n"
                "• Двойной щелчок левой кнопкой — добавить точку / удалить точку.\n"
                "• Щелчок правой кнопкой — добавить точку / удалить точку.\n"
                "• Перетаскивание — двигать точку.\n"
                "• Shift + перетаскивание — точка двигается только по диагонали от 0,0 до 127,127.\n"
                "• Крайние точки (на x = 0 и x = 127) удалить нельзя, они двигаются только вверх-вниз.\n"
                "• Координаты выбранной точки показаны справа, в блоке «Точка». Их можно вписать "
                "вручную (от 0 до 127) и нажать Enter.\n"
                "\n"
                "# ИЗГИБ ОТРЕЗКА\n"
                "• Посередине каждого отрезка есть маленький кружок. Потяните его вверх или вниз — "
                "отрезок станет кривой.\n"
                "• Двойной щелчок по кружку или щелчок правой кнопкой — снова прямая.\n"
                "\n"
                "# ПРОФИЛИ\n"
                "• Профиль — это файл .json в папке профилей (путь указан справа). "
                "В списке «Профиль» — все файлы из этой папки.\n"
                "• Выбор профиля в списке загружает его кривую. Если выбрать тот же профиль ещё раз, "
                "кривая снова загрузится из файла, а несохранённые изменения пропадут.\n"
                "• «Сохранить текущую кривую» — записать кривую в выбранный профиль. Если профиль "
                "не выбран, плагин спросит название.\n"
                "• «Сохранить как» — сохранить кривую в новый профиль.\n"
                "• «Импорт» — выбрать .json в любом месте на диске: он скопируется в папку профилей "
                "и загрузится.\n"
                "• «Экспорт» — сохранить кривую в .json в любое место.\n"
                "• «Папка» — открыть папку профилей. Ненужные профили удаляются вручную в этой папке.\n"
                "• Файл профиля можно править в любом текстовом редакторе. x и y — от 0 до 127, "
                "curve — изгиб отрезка от этой точки до следующей: 0 — прямая, больше 0 — выгнут "
                "вверх, меньше 0 — вниз, от -10 до 10.\n"
                "• Текущая кривая сохраняется и в проекте DAW, даже без профиля.\n"
                "\n"
                "# ПРОЧЕЕ\n"
                "• «Сброс» — вернуть прямую от 0,0 до 127,127.\n"
                "• «Последнее сообщение»: In — velocity последней пришедшей ноты, Out — velocity, "
                "с которой она ушла.\n"
                "• Окно плагина растягивается за правый нижний угол, пропорции сохраняются.");
        }
        else
        {
            t.lastMessage  = u ("Last Message");
            t.in           = u ("In");
            t.out          = u ("Out");
            t.point        = u ("Point");
            t.pointHint    = u ("Select or add a point");
            t.profile      = u ("Profile");
            t.noProfile    = u ("— none —");
            t.emptyFolder  = u ("No profiles in the folder");
            t.saveCurrent  = u ("Save current curve");
            t.saveAs       = u ("Save as");
            t.import       = u ("Import");
            t.exportCurve  = u ("Export");
            t.folder       = u ("Folder");
            t.profilesPath = u ("Profiles folder:");
            t.reset        = u ("Reset");
            t.help         = u ("Help");
            t.close        = u ("Close");
            t.saveAsTitle  = u ("Save as");
            t.profileName  = u ("Profile name:");
            t.ok           = u ("OK");
            t.cancel       = u ("Cancel");
            t.yes          = u ("Replace");
            t.no           = u ("Cancel");
            t.replaceTitle = u ("Profile exists");
            t.replaceMessage = u ("Profile \"%1\" already exists. Replace it?");
            t.errorTitle   = u ("Error");
            t.readError    = u ("Could not read the file \"%1\".");
            t.badJson      = u ("This is not JSON or it has an error: %1");
            t.noPoints     = u ("The file has no \"points\" list.");
            t.badPoint     = u ("Point #%1 has no \"x\" or \"y\".");
            t.writeError   = u ("Could not write the file \"%1\".");
            t.importTitle  = u ("Import profile");
            t.exportTitle  = u ("Export curve");
            t.jsonHelp     = u ("x is the incoming velocity, y is the outgoing one, both from 0 to 127. "
                                "curve is the bend of the segment from this point to the next one: "
                                "0 is straight, above 0 bends up, below 0 bends down, from -10 to 10. "
                                "The first point must be at x = 0, the last one at x = 127.");

            t.helpText = u (
                "# HOW IT WORKS\n"
                "MidiCurveVST changes the velocity of played notes according to the curve. "
                "Horizontal axis — incoming velocity (In), vertical axis — outgoing velocity (Out). "
                "Everything else (note releases, CC, pitch bend, etc.) passes unchanged. "
                "If the curve gives 0, the note is sent with velocity 0.\n"
                "To get the notes to an instrument, choose a MIDI output port in MidiCurveVST "
                "and the same input port in the instrument.\n"
                "\n"
                "# POINTS\n"
                "• Left double-click — add / delete a point.\n"
                "• Right click — add / delete a point.\n"
                "• Drag — move a point.\n"
                "• Shift + drag — the point moves only along the diagonal from 0,0 to 127,127.\n"
                "• The end points (at x = 0 and x = 127) can't be deleted and move only up and down.\n"
                "• The coordinates of the selected point are shown on the right, in the \"Point\" block. "
                "You can type them in (0 to 127) and press Enter.\n"
                "\n"
                "# SEGMENT BEND\n"
                "• Every segment has a small circle in its middle. Drag it up or down to bend the segment.\n"
                "• Double-click the circle or right-click it to make the segment straight again.\n"
                "\n"
                "# PROFILES\n"
                "• A profile is a .json file in the profiles folder (the path is shown on the right). "
                "The \"Profile\" list shows all files from this folder.\n"
                "• Choosing a profile in the list loads its curve. Choosing the same profile again "
                "reloads the curve from the file; unsaved changes are lost.\n"
                "• \"Save current curve\" — write the curve to the selected profile. If no profile is "
                "selected, the plugin asks for a name.\n"
                "• \"Save as\" — save the curve as a new profile.\n"
                "• \"Import\" — choose a .json anywhere on disk: it is copied to the profiles folder and loaded.\n"
                "• \"Export\" — save the curve to a .json anywhere.\n"
                "• \"Folder\" — open the profiles folder. Delete unneeded profiles there by hand.\n"
                "• A profile file can be edited in any text editor. x and y are 0 to 127, curve is the "
                "bend of the segment from this point to the next: 0 is straight, above 0 bends up, "
                "below 0 bends down, from -10 to 10.\n"
                "• The current curve is also saved in the DAW project, even without a profile.\n"
                "\n"
                "# OTHER\n"
                "• \"Reset\" — back to a straight line from 0,0 to 127,127.\n"
                "• \"Last Message\": In — velocity of the last incoming note, Out — velocity it was sent with.\n"
                "• The plugin window resizes from the bottom-right corner, keeping its proportions.");
        }

        return t;
    }
};
