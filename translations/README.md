# Menu translations

The port's own menus (F1, Remap, the data folder and the other overlays) in the
system's language. Each `<language>.json` maps an English text the menus show to
its translation:

```json
{
 "Fullscreen (F11)": "Plein écran (F11)",
 "Checking the disc image (%.0f%%)...": "Vérification de l'image disque (%.0f %%)..."
}
```

To add a language, copy `fr.json` to `<language>.json` (the two-letter code the
system reports: `de`, `es`, `it`...), translate the values, and rebuild. The
files are picked up by `cmake/PortTranslations.cmake`; nothing else to register.

- The language is the first of the system's preferred languages that has a
  file here, unless English comes first (on Android, the phone's language).
  `MP2_UI_LANGUAGE=<language>` chooses for one run (`MP2_UI_LANGUAGE=en` for
  English).
- The files are built into the executable (`cmake/PortTranslations.cmake`), so
  a change needs a rebuild.
- ImGui looks up every text it measures, draws or formats
  (`extern/aurora/extern/imgui_translate.cmake`, `platform/port_translate.cpp`):
  the code keeps its English labels, which stay the widgets' ids and what the
  settings search finds. A key is the text as shown, without the `##id` part;
  the items of a combo are separate texts.
- A text with no translation shows in English, and so does a text the code
  builds from pieces at run time.
- A text that is a printf format keeps its conversions, in the same order
  (`%d`, `%.1f`, `%s`, `%%`); a translation that differs is left out, with a
  line in the log.
- The menu font is guaranteed to have Latin-1 only (ImGui's built-in one; other
  ranges are merged from a system font when one is found): no `œ`, curly quotes,
  `…` or dashes beyond `-`. Use `...` and `'`.
- The menus' terms follow the Metroid Prime port's French file, so the two ports
  read alike (Rayon, Visière, Morphing Ball, Attributions...).
