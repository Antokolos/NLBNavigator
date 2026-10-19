package com.nlbhub.nlbnavigator;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.libsdl.app.SDLActivity;

/**
 * Плеер книги NLB: SDLActivity, запускающая нативный nlb_sdl (libmain.so).
 * <ul>
 * <li>Вариант game: книга встроена в приложение (assets/book.nlbpack), параметры плеера заданы
 * при сборке (gradle.properties, nlb.game.args).</li>
 * <li>Вариант player: книгу и её параметры передаёт лаунчер — EXTRA_BOOK (путь к .nlbpack) и
 * EXTRA_ARGS (режим vn/standard и ключи).</li>
 * </ul>
 */
public class NLBActivity extends SDLActivity {
    public static final String EXTRA_BOOK = "com.nlbhub.nlbnavigator.BOOK";
    public static final String EXTRA_ARGS = "com.nlbhub.nlbnavigator.ARGS";

    @Override
    protected String[] getLibraries() {
        // SDL3 — разделяемая библиотека, SDL_image/ttf/mixer и плеер — внутри libmain.so
        return new String[] {"SDL3", "main"};
    }

    @Override
    protected String[] getArguments() {
        final List<String> args = new ArrayList<>();
        final String book = getIntent() != null ? getIntent().getStringExtra(EXTRA_BOOK) : null;
        if (book != null && !book.isEmpty()) args.add(book);
        String extra = getIntent() != null ? getIntent().getStringExtra(EXTRA_ARGS) : null;
        if (extra == null) extra = BuildConfig.NLB_ARGS;
        extra = extra.trim();
        if (!extra.isEmpty()) args.addAll(Arrays.asList(extra.split("\\s+")));
        return args.toArray(new String[0]);
    }
}
