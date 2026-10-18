package com.nlbhub.nlbnavigator;

import org.libsdl.app.SDLActivity;

/**
 * Плеер книги NLB: SDLActivity, запускающая нативный nlb_sdl (libmain.so).
 * В варианте game книга встроена в приложение (assets/book.nlbpack), а параметры плеера
 * (режим vn/standard и ключи) заданы при сборке — gradle.properties, nlb.game.args.
 */
public class NLBActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        // SDL3 — разделяемая библиотека, SDL_image/ttf/mixer и плеер — внутри libmain.so
        return new String[] {"SDL3", "main"};
    }

    @Override
    protected String[] getArguments() {
        final String args = BuildConfig.NLB_ARGS.trim();
        return args.isEmpty() ? new String[0] : args.split("\\s+");
    }
}
