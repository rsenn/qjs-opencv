# tools/site — the qjs-opencv GitHub Pages site

Generates <https://rsenn.github.io/qjs-opencv/> from the repo's own markdown.
No toolchain beyond `qjsm` itself: the markdown renderer and the syntax
highlighter are in here.

```sh
qjsm tools/site/build.js          # -> _site/
qjsm tools/site/build.js /tmp/out # or anywhere else
```

| File | Role |
|------|------|
| `build.js` | site map, link rewriting, page shell, entry point |
| `markdown.js` | CommonMark/GFM subset renderer (see its header for what's deliberately missing) |
| `highlight.js` | js / sh / c tokenizer for fenced blocks |
| `landing.html` | hand-written landing page body; `<x-code lang="…">` blocks go through `highlight.js` |
| `style.css` | one stylesheet, light and dark |
| `favicon.svg` | tab icon |

`markdown.js`, `highlight.js` and `style.css` are shared verbatim with the
sister project [qjs-lws](https://github.com/rsenn/qjs-lws)'s `tools/site` —
fix a renderer bug in one, copy it to the other.

## Adding or moving a doc page

`NAV` at the top of `build.js` is the whole site map: each entry is
`[markdown source, output path, sidebar label]`. Adding a `doc/**.md` file
without listing it there means it does not get built, and inter-doc links
pointing at it fall back to a github.com blob URL instead of a site page.

`doc/opencv-js-api.md` and `doc/opencv-js-examples.md` are deliberately not
listed: they document upstream `opencv.js` — the porting target — rather than
this project, and cite local checkout paths.

Everything is linked with relative paths, so the output works both under
the project-pages prefix and from a local `file://` checkout.

## Publishing

`.github/workflows/pages.yml` rebuilds the site on every push to `main` that
touches `README.md`, `TODO.md`, `doc/` or `tools/site/`, and force-pushes the
result to the orphan `gh-pages` branch, which holds only generated output.

To publish by hand instead:

```sh
git worktree add ../qjs-opencv-pages gh-pages
qjsm tools/site/build.js ../qjs-opencv-pages
cd ../qjs-opencv-pages && git add -A && git commit -m 'Rebuild site' && git push
```

`.nojekyll` is emitted by the build so GitHub serves the files as-is.
