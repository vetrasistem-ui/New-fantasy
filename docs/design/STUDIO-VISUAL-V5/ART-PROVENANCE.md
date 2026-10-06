# Fantasy Studio V5 — presentation art provenance

Date: 2026-10-06.

These two images are original presentation assets for the Fantasy Studio shell. They were generated and subsequently refined with the built-in `image_gen.imagegen` tool, using the imagegen skill in its default built-in mode. No CLI, API key, downloaded third-party art, screenshots, styleboard crops or reference pixels are used as application textures.

## Scope and references

The approved `Fantasy-Studio-Visual-Package-V4` reference images were inspected before generation:

- `references/01_branding_logo.png`: visual guidance for the silver/blue F-and-sword emblem with a gold gemstone.
- `references/04_home_target.png`: composition and color guidance for the landscape in the Home hero.
- `references/00_styleboard_full.png`: overall approved design language, supplied by the owner.

The supplied images are design references only. They were not edited, cropped or copied into these generated images. The emblem and hero are separate shell presentation art; they are not game content, sprites, map tiles, FMAP previews, items, imported assets or evidence of any future integration.

## Final files

| File | Dimensions | Mode | SHA-256 |
| --- | --- | --- | --- |
| `Studio/UI/Assets/fantasy-emblem-v5.png` | 1024 × 1536 | RGBA | `d2905def46ee7d6cddd031230ddae1a60c0b2b02c84b00677b77a154fab0b68c` |
| `Studio/UI/Assets/fantasy-hero-v5.png` | 2172 × 724 | RGB | `312a27d6803effd334da784fa1e7598f06e9757ee1f61f1aeca95c9a7fcb6baf` |

The final built-in generated/edited outputs were copied directly into the workspace without manual pixel edits, cropping, background removal or reencoding. The emblem's generated alpha was preserved: 1,023,204 pixels are fully transparent; the alpha bounding box is `(49, 6, 1010, 1505)`. Read-only inspection with Pillow checked dimensions, mode and alpha; Pillow was not used to edit either image.

These assets may be scaled and composed by the Studio UI at runtime. They do not belong in `Game/` and must not be presented as actual map or asset-library content.

## Initial emblem generation prompt

```text
Use case: logo-brand.
Asset type: final standalone presentation emblem for the Fantasy Studio desktop application shell, PNG with genuine transparent background.
Primary request: Create a NEW, finely rendered metallic Fantasy Studio emblem, with very strong visual correspondence to the approved branding reference shown as Image 1. Reconstruct the emblem's visual language, not a crop, screenshot, logo sheet, UI panel or copied pixels.
Subject: A tall, angular capital F fused with a downward pointed sword. A long faceted sapphire-blue blade forms the main vertical stem and pointed bottom. The large upper F crossbar is polished silver and electric blue, with beveled angular ends. A shorter blue beveled middle crossbar projects right. At the top center a small luminous silver diamond/crystal finial rises above the guard. Immediately under the finial is one small warm gold diamond gemstone held by a silver diamond surround. Deep blue dark metallic inset shapes and silver bevels reinforce the recognizable F silhouette.
Composition/framing: Single emblem centered, all edges and the long bottom tip visible, occupying most of a 1024x1536 portrait canvas with modest clear margins. Sharp clean silhouette; no text anywhere.
Style/medium: Premium polished fantasy game creation software brand mark, richly shaded 3D beveled metal illustration, faithful to the reference's heroic silver/cyan/sapphire material, not flat vector, not abstract geometry, not pixel art.
Lighting/mood: Crisp cool highlights on metal facets, dark navy shadows within the emblem, tiny restrained cyan rim light, minimal glow and no background halo.
Color palette: Silver-white steel, sapphire and electric-blue metal, cyan edge highlights, one gold center gemstone.
Constraints: Actual fully transparent alpha background. No background rectangle, no checkerboard baked into pixels, no scenery, no badge box, no circle, no decorative text, no lettering beyond the emblem's F construction, no watermark, no extra symbols. New standalone emblem only.
```

Tool arguments: `transparent_background=true`. New-image generation; no edit target was supplied.

Inspection: recognizable tall F/sword silhouette, blue and silver beveled metal, central gold gemstone, long pointed blade, no text. True transparency was verified from the output file.

## Initial hero generation prompt

```text
Use case: stylized-concept.
Asset type: final original Home hero illustration for Fantasy Studio desktop editor, a very wide presentation banner, not a map or gameplay asset.
Primary request: Generate a NEW fantasy landscape illustration with strong immediate visual correspondence to the hero inside the approved Home reference Image 2. Do not create a UI screenshot or embed any part of the supplied reference image.
Scene/backdrop: A luminous green valley with a monumental blue-gray gothic fantasy castle on a grassy, sunlit rocky hill toward the right side. Tall tower clusters, slender spires and crisp blue stone architecture. Behind it layered blue mountains, a brilliant cyan-blue sky and white cumulus clouds. Green grass, sparse dark evergreen trees and sunny yellow-green slopes in the foreground. Rich polished hand-painted fantasy game software presentation art.
Composition/framing: Very wide landscape, preferably 3:1 (1536x512 or a wider banner that allows this crop). The leftmost 40 percent must be intentionally dark deep navy negative space, smooth and quiet, with only barely visible atmospheric mountain silhouettes, suitable for independently rendered metallic branding and white UI text. Transition from navy at the left through shadowed blue mountains around the middle to bright sky, grass and a clearly recognizable large blue castle occupying the right third. Castle spires remain inside the composition, no clipping. Strong horizontal composition suited to a short desktop app hero.
Style/medium: Crisp detailed painterly fantasy illustration with clean silhouettes and rich surface shading, premium game creation software presentation art. Use the reference's bright adventurous castle/valley motif and blue/green/gold colors; not retro pixel art, not wireframe, not abstract geometric mountain icons, not a photorealistic photograph.
Lighting/mood: Clear cheerful daylight on the right, bright blue sky with puffy white clouds, high contrast lit grass, atmospheric blue distance. Deep navy shading on the left for readable text, gradual tasteful transition.
Constraints: No text, no letters, no logo, no people, no characters, no creatures, no swords, no cards, no interface, no watermark, no fake map, no item sprites, no icons, no screenshot border. This is exclusively decorative branding art, separate from any map, FMAP content or imported assets.
```

Tool arguments: `transparent_background=false`. New-image generation; no edit target was supplied.

Inspection: wide 3:1 composition, dark navy space on the left, detailed sunlit castle on the right, cyan sky, mountains and green slopes; no text, UI, characters or sprites.

## Owner-requested fidelity refinement

The owner requested closer correspondence after rejecting the previous appearance. The original generated files were edited through the built-in tool with their respective approved graphics as visual reference inputs. Reference input role was explicit: design guidance only; no cropping, extraction or compositing of screenshot pixels. Only these two presentation PNGs were replaced. Font, license and core were not changed.

Initial generated output hashes, retained here as provenance history:

- Emblem: `5c18657befcd363cb9721e8adf69ef7b87beedab4bc958b1e99dbe06827ae099`.
- Hero: `fe291c33530ab6a7e88e0b5a0ef648e6c41b5821b87be64fc7a7e4dd996a3cb8`.

The final hero uses stylized painted forms and saturated daylight colors, with dark blue/teal castle stone. The final emblem has a broader blade and straight angular crossbars. They remain original art; neither is an exact reproduction of the approved emblem or castle geometry.

### Hero style edit prompt

Inputs: the initially generated hero as edit target; `references/04_home_target.png` as a design reference. Tool argument: `transparent_background=false`.

```text
Use case: style-transfer.
Asset type: Original decorative Fantasy Studio Home hero PNG, not game content.
Input images: Image 1 is the existing original hero to edit. Image 2 is a visual design reference only; use ONLY the landscape illustration's composition, palette and rendering language inside its top hero. It is NOT a target image to crop, replicate as a screenshot, composite or extract pixels from.
Primary request: Transform Image 1 into a brighter, more stylized illustrated fantasy game landscape that immediately resembles the approved reference hero's art language. The current cinematic near-photographic detailing must be replaced by crisply painted stylized forms, bold bright color planes, clear illustrated edges and purposeful simplified details.
Scene: A monumental fantasy castle at the right on a sunny green rocky hill, blue stone towers and steep blue spires, pine trees on the hillside, angular layered distant mountains, brilliant sky blue and lively white cumulus clouds. Castle silhouette, hill, mountains and clouds should feel like premium hand-painted 2D fantasy game concept illustration, not photography or a realistic matte painting. Rich saturated grass greens and yellow-gold highlights, cool deep-blue castle shadows, clear azure/cyan sky.
Composition/framing: Keep the source wide 3:1 landscape format. Rebuild the castle as a compact imposing blue castle grouped in the rightmost quarter, with grass hills and trees below. Mountains/clouds occupy the middle third. Leftmost 38 percent is quiet deep navy negative space for separately rendered branding/text; no objects cover that zone. The illustrated scene should extend all the way to the right edge. Keep all castle towers visible.
Constraints: Edit Image 1 only. Do not include or copy any UI, text, logo, panels, cards, screenshot borders or other parts of Image 2. No text, logo, symbols, people, characters, item icons, tile sprites or fake game content. This is original presentation art only. No photorealistic materials, tiny realistic texture noise, wireframe or geometric-placeholder appearance. Preserve the decorative navy-left / bright-castle-right composition and create an unmistakably illustrated result.
```

### Hero castle palette edit prompt

Inputs: the hero produced by the preceding style edit as edit target; `references/04_home_target.png` as a design reference. Tool argument: `transparent_background=false`.

```text
Use case: precise-object-edit.
Input images: Image 1 is the edited original hero to change. Image 2 is the approved Home design reference only, not a screenshot to crop or insert.
Change only the castle architecture's palette and silhouette in Image 1 to closely match the dark blue/teal castle in the hero landscape of Image 2. Make the castle a clustered monumental DARK sapphire-blue stone fortress with angular cyan-blue edge highlights and navy pointed spires. The walls must be dark blue/teal stone, NOT cream, beige, pale gray or white. Keep the sunlit lime/yellow-green hill, bright saturated illustrated cyan sky and white cloud forms, stylized pine trees, painted mountains and the deep-navy negative space on the left exactly as the edited composition. Maintain the wide 3:1 image, overall castle position in the right third, and all current landscape framing.
Style: crisp hand-painted stylized fantasy game concept illustration, premium adventurous software hero. Preserve the illustrative rendering; no photorealism.
No text, logo, UI, border, panels, screenshots, sprites, characters or additional objects. Image 2 supplies art direction only; never crop, copy or composite pixels from it.
```

### Emblem geometry edit prompt

Inputs: the initially generated emblem as edit target; `references/01_branding_logo.png` as a design reference. Tool argument: `transparent_background=true`.

```text
Use case: style-transfer.
Asset type: Original standalone Fantasy Studio branding emblem PNG with genuine transparent alpha.
Input images: Image 1 is the existing original emblem to edit. Image 2 is the approved branding design reference only; its F/sword symbol supplies geometry, proportions and metallic color guidance. Do not crop, extract or composite any pixels or text from Image 2.
Primary request: Rebuild the emblem in Image 1 into a substantially bulkier, angular, broad-bladed F fused with a sword, with strong visual correspondence to the F/sword symbol in Image 2. The current curvy thin elongated filigree letter must become compact heroic sword geometry.
Subject/details: Thick broad faceted sapphire-blue sword blade forms the F's vertical body, with crisp bright cyan central bevel and a pointed bottom. Upper horizontal F bar is a wide straight blue/silver sword guard with sharp bevel planes and short pointed downturned ends. The shorter thick rectangular middle F bar projects to the right with a flat strong blue face and bright silver edges. Keep dark navy metal inset framing behind the main broad blade. Small diamond/crystal finial above the guard. One small gold diamond jewel with a silver diamond mount at the top center. Broad clean metallic facets and straight angular geometry, vivid blue planes and silver highlights like the approved emblem.
Geometry: Increase the thick central blade and all strokes to have solid visual weight. Reduce the tall finial to a small diamond. Shorten the excessively long thin blade and retain a long sharp but broad bottom point. No curved calligraphic flourishes, scrolls, scallops, thin stems or decorative letter serifs. Strong squared F crossbars, sturdy sword form, large facets readable at app-icon and hero-logo sizes.
Composition/framing: Single entire emblem centered on a portrait 1024x1536 canvas, completely visible with modest clear margins. The emblem should have a wider/bulkier body and distinct F silhouette, no other content.
Materials/lighting: Polished sapphire/electric-blue metal, silver bevels, cyan highlights, deep blue shadow faces, central gold jewel. Crisp restrained rim highlights; minimal glow.
Constraints: Edit the original generated emblem only. Genuine transparent background with alpha, preserve clean transparency and do not add a backdrop, halo, checkerboard, rectangle or badge. No text, no words, no wordmark, no scenery, no characters, no UI or screenshot border. Do not crop or use Image 2 pixels.
```

## Branding font — Cinzel

The unmodified Cinzel variable TrueType font was downloaded from the official Google Fonts repository for branding only. It is loaded by the application; it was not installed in the operating system. Neutral controls and interface copy continue to use the system UI font.

- Official source: [google/fonts — Cinzel](https://github.com/google/fonts/tree/main/ofl/cinzel).
- Font source: [Cinzel variable font](https://raw.githubusercontent.com/google/fonts/main/ofl/cinzel/Cinzel%5Bwght%5D.ttf).
- License source: [OFL.txt](https://raw.githubusercontent.com/google/fonts/main/ofl/cinzel/OFL.txt).
- Font file: `Studio/UI/Assets/Fonts/Cinzel.ttf`, 125,468 bytes, SHA-256 `f4d83d34d1f6c741193e4acf4b3dff9531e5a67b6aa65228d00a7db72a4e0f34`.
- Preserved license: `Studio/UI/Assets/Fonts/OFL-Cinzel.txt`, 4,383 bytes, SHA-256 `f2b3029aba64c378bf0963b62945eee15e564fe4330b934c8f2eb058282b5e83`.

The license was read: SIL Open Font License 1.1 permits use and bundling with software when copyright and the license are preserved. The font must retain that license and cannot be sold alone. The downloaded font binary is unchanged; only its local filename is normalized. The bundled notice identifies Copyright 2020 The Cinzel Project Authors. A read-only font parse identified `Cinzel / Regular`; its TrueType signature is `00010000` with 21 tables.

This is a presentation font, not third-party core code or a map/protocol dependency. The inclusion is also recorded in `docs/UPSTREAMS.md`.

## Acceptance

Generated presentation art does not establish visual PASS. Final Studio screenshots and the owner's visual approval remain the acceptance gate. No F05.5, F06, OTBM, DAT/SPR/OTB or real-sprite integration is introduced by these files.
