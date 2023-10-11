#include "audio_options.h"

#include "../controls/controller.h"
#include "../savefile/savefile.h"
#include "../font/dejavusans.h"
#include "../audio/soundplayer.h"
#include "../build/src/audio/subtitles.h"
#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"

#define GAMEPLAY_Y      54
#define GAMEPLAY_WIDTH  252
#define GAMEPLAY_HEIGHT 124
#define GAMEPLAY_X      ((SCREEN_WD - GAMEPLAY_WIDTH) / 2)

#define SCROLL_TICKS        10//(int)maxf(NUM_SUBTITLE_LANGUAGES, 1)
#define SCROLL_INTERVALS    (int)maxf((SCROLL_TICKS - 1), 1)
#define FULL_SCROLL_TIME    2.0f
#define SCROLL_MULTIPLIER   (int)(0xFFFF * FIXED_DELTA_TIME / (80 * FULL_SCROLL_TIME))
#define SCROLL_CHUNK_SIZE   (0x10000 / SCROLL_INTERVALS)

void audioOptionsHandleSlider(unsigned short* settingValue, float* sliderValue) {
    OSContPad* pad = controllersGetControllerData(0);

    int newValue = (int)*settingValue + pad->stick_x * SCROLL_MULTIPLIER;

    if (controllerGetButtonDown(0, A_BUTTON | R_JPAD)) {
        if (newValue >= 0xFFFF && controllerGetButtonDown(0, A_BUTTON)) {
            newValue = 0;
        } else {
            newValue = newValue + SCROLL_CHUNK_SIZE;
            newValue = newValue - (newValue % SCROLL_CHUNK_SIZE);
        }
    }

    if (controllerGetButtonDown(0, L_JPAD)) {
        newValue = newValue - 1;
        newValue = newValue - (newValue % SCROLL_CHUNK_SIZE);
    }

    if (newValue < 0) {
        newValue = 0;
    }

    if (newValue > 0xFFFF) {
        newValue = 0xFFFF;
    }

    *settingValue = newValue;
    *sliderValue = (float)newValue / 0xFFFF;
}

void audioOptionsInit(struct AudioOptions* audioOptions) {
    audioOptions->selectedItem = AudioOptionGameVolume;

    audioOptions->gameVolumeText = menuBuildText(&gDejaVuSansFont, "Master Volume", GAMEPLAY_X + 8, GAMEPLAY_Y + 8);
    audioOptions->gameVolume = menuBuildSlider(GAMEPLAY_X + 120, GAMEPLAY_Y + 8, 120, SCROLL_TICKS);
    audioOptions->gameVolume.value = gSaveData.audio.soundVolume/0xFFFF;

    audioOptions->musicVolumeText = menuBuildText(&gDejaVuSansFont, "Music Volume", GAMEPLAY_X + 8, GAMEPLAY_Y + 28);
    audioOptions->musicVolume = menuBuildSlider(GAMEPLAY_X + 120, GAMEPLAY_Y + 28, 120, SCROLL_TICKS);
    audioOptions->musicVolume.value = gSaveData.audio.musicVolume/0xFFFF;

    audioOptions->subtitlesEnabled = menuBuildCheckbox(&gDejaVuSansFont, "Closed Captions", GAMEPLAY_X + 8, GAMEPLAY_Y + 48);
    audioOptions->subtitlesEnabled.checked = (gSaveData.controls.flags & ControlSaveSubtitlesEnabled) != 0;

    audioOptions->subtitlesLanguageText = menuBuildText(&gDejaVuSansFont, "Captions Language: ", GAMEPLAY_X + 8, GAMEPLAY_Y + 68);
    audioOptions->subtitlesLanguageDynamicText = menuBuildText(&gDejaVuSansFont, SubtitleLanguages[gSaveData.controls.subtitleLanguage], GAMEPLAY_X + 125, GAMEPLAY_Y + 68);

    audioOptions->subtitlesLanguage= menuBuildSlider(GAMEPLAY_X + 8, GAMEPLAY_Y + 88, 200, NUM_SUBTITLE_LANGUAGES);
    audioOptions->subtitles_language_temp = (0xFFFF/NUM_SUBTITLE_LANGUAGES)* gSaveData.controls.subtitleLanguage;
    audioOptions->subtitlesLanguage.value = (int)gSaveData.controls.subtitleLanguage * (0xFFFF/NUM_SUBTITLE_LANGUAGES);
}

enum MenuDirection audioOptionsUpdate(struct AudioOptions* audioOptions) {
    int controllerDir = controllerGetDirectionDown(0);

    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }

    if (controllerDir & ControllerDirectionDown) {
        ++audioOptions->selectedItem;

        if (audioOptions->selectedItem == AudioOptionCount) {
            audioOptions->selectedItem = 0;
        }
    }

    if (controllerDir & ControllerDirectionUp) {
        if (audioOptions->selectedItem == 0) {
            audioOptions->selectedItem = AudioOptionCount - 1;
        } else {
            --audioOptions->selectedItem;
        }
    }

    switch (audioOptions->selectedItem) {
        case AudioOptionGameVolume:
            audioOptionsHandleSlider(&gSaveData.audio.soundVolume, &audioOptions->gameVolume.value);
            soundPlayerGameVolumeUpdate(SoundTypeAll);
            break;
        case AudioOptionMusicVolume:
            audioOptionsHandleSlider(&gSaveData.audio.musicVolume, &audioOptions->musicVolume.value);
            soundPlayerGameVolumeUpdate(SoundTypeMusic);
            break;
        case AudioOptionSubtitlesEnabled:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                audioOptions->subtitlesEnabled.checked = !audioOptions->subtitlesEnabled.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

                if (audioOptions->subtitlesEnabled.checked) {
                    gSaveData.controls.flags |= ControlSaveSubtitlesEnabled;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveSubtitlesEnabled;
                }
            }
            break;
        case AudioOptionSubtitlesLanguage:
            audioOptionsHandleSlider(&audioOptions->subtitles_language_temp, &audioOptions->subtitlesLanguage.value);
            int temp = (int)((audioOptions->subtitles_language_temp * (1.0f/0xFFFF) * NUM_SUBTITLE_LANGUAGES));
            temp = (int)minf(maxf(0.0, temp), NUM_SUBTITLE_LANGUAGES-1);
            gSaveData.controls.subtitleLanguage = temp;
            audioOptions->subtitlesLanguageDynamicText = menuBuildText(&gDejaVuSansFont, SubtitleLanguages[gSaveData.controls.subtitleLanguage], GAMEPLAY_X + 125, GAMEPLAY_Y + 68);
            break;
    }
    

    if (audioOptions->selectedItem == AudioOptionSubtitlesLanguage || 
        audioOptions->selectedItem == AudioOptionGameVolume ||
        audioOptions->selectedItem == AudioOptionMusicVolume){
        if ((controllerGetButtonDown(0, L_TRIG) || controllerGetButtonDown(0, Z_TRIG))) {
            return MenuDirectionLeft;
        }
        if ((controllerGetButtonDown(0, R_TRIG))) {
            return MenuDirectionRight;
        }
    }
    else{
        if (controllerDir & ControllerDirectionLeft || controllerGetButtonDown(0, L_TRIG) || controllerGetButtonDown(0, Z_TRIG)) {
            return MenuDirectionLeft;
        }
        if (controllerDir & ControllerDirectionRight || controllerGetButtonDown(0, R_TRIG)) {
            return MenuDirectionRight;
        }
    }

    return MenuDirectionStay;
}

void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, audioOptions->gameVolume.back);
    renderState->dl = menuSliderRender(&audioOptions->gameVolume, renderState->dl);

    gSPDisplayList(renderState->dl++, audioOptions->musicVolume.back);
    renderState->dl = menuSliderRender(&audioOptions->musicVolume, renderState->dl);
    

    gSPDisplayList(renderState->dl++, audioOptions->subtitlesEnabled.outline);
    renderState->dl = menuCheckboxRender(&audioOptions->subtitlesEnabled, renderState->dl);

    gSPDisplayList(renderState->dl++, audioOptions->subtitlesLanguage.back);
    renderState->dl = menuSliderRender(&audioOptions->subtitlesLanguage, renderState->dl);


    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, audioOptions->selectedItem == AudioOptionGameVolume, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, audioOptions->gameVolumeText);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, audioOptions->selectedItem == AudioOptionMusicVolume, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, audioOptions->musicVolumeText);


    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, audioOptions->selectedItem == AudioOptionSubtitlesEnabled, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, audioOptions->subtitlesEnabled.text);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, audioOptions->selectedItem == AudioOptionSubtitlesLanguage, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, audioOptions->subtitlesLanguageText);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, audioOptions->selectedItem == AudioOptionSubtitlesLanguage, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, audioOptions->subtitlesLanguageDynamicText);


    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}