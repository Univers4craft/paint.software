#include "i18n.h"

#include <QHash>
#include <QSettings>
#include <QTranslator>
#include <QCoreApplication>
#include <QLibraryInfo>

namespace {

// Matches the default in loadFromSettings(), for any code path that runs before it.
I18n::Lang g_lang = I18n::Lang::English;

// French (source) -> English. Ampersands mark menu accelerators and are kept
// in both languages.
const QHash<QString, QString> &table() {
    static const QHash<QString, QString> m = {
        // ---- Menu bar ----
        {"&Fichier", "&File"},
        {"É&dition", "&Edit"},
        {"&Affichage", "&View"},
        {"&Image", "&Image"},
        {"&Calques", "&Layers"},
        {"A&justements", "&Adjustments"},
        {"E&ffets", "Effe&cts"},
        {"&Options", "&Options"},

        // ---- File menu ----
        {"&Nouveau...", "&New..."},
        {"&Ouvrir...", "&Open..."},
        {"Fichiers &récents", "&Recent files"},
        {"(aucun)", "(none)"},
        {"Effacer la liste", "Clear list"},
        {"&Enregistrer", "&Save"},
        {"Enregistrer &sous...", "Save &As..."},
        {"Im&primer...", "&Print..."},
        {"&Fermer l'image", "&Close image"},
        {"&Quitter", "E&xit"},

        // ---- Edit menu ----
        {"&Annuler", "&Undo"},
        {"&Rétablir", "&Redo"},
        {"C&ouper", "Cu&t"},
        {"&Copier", "&Copy"},
        {"Co&ller", "&Paste"},
        {"&Supprimer", "&Delete"},
        {"&Tout sélectionner", "Select &All"},
        {"&Désélectionner", "&Deselect All"},
        {"&Inverser la sélection", "&Invert Selection"},

        // ---- View menu ----
        {"Zoom a&vant", "Zoom &In"},
        {"Zoom a&rrière", "Zoom &Out"},
        {"Zoom &adapté", "&Best Fit"},
        {"Taille &réelle", "&Actual Size"},
        {"Afficher la &grille de pixels", "&Pixel Grid"},
        {"&Règles", "&Rulers"},

        // ---- Image menu ----
        {"&Redimensionner...", "&Resize..."},
        {"Taille du &canevas...", "&Canvas Size..."},
        {"Rotation 90° horaire", "Rotate 90° Clockwise"},
        {"Rotation 90° anti-horaire", "Rotate 90° Counter-Clockwise"},
        {"Rotation 180°", "Rotate 180°"},
        {"Retourner &horizontalement", "Flip &Horizontal"},
        {"Retourner &verticalement", "Flip &Vertical"},
        {"Rogner selon la sélection", "Crop to Selection"},
        {"&Aplatir", "&Flatten"},

        // ---- Layers menu / panel ----
        {"&Propriétés du calque...", "Layer &Properties..."},
        {"Ajouter un calque", "Add New Layer"},
        {"Supprimer le calque", "Delete Layer"},
        {"Dupliquer le calque", "Duplicate Layer"},
        {"Fusionner vers le bas", "Merge Layer Down"},
        {"Monter le calque", "Move Layer Up"},
        {"Descendre le calque", "Move Layer Down"},
        {"Basculer la visibilité", "Toggle Visibility"},
        {"Calque collé", "Pasted Layer"},
        {"Tous", "All"},
        {"Appliquer les modifications a tous les calques", "Apply changes to all layers"},
        {"Veuillez créer un document avant d'ajouter un calque.",
         "Please create a document before adding a layer."},

        // ---- Adjustments ----
        {"Niveau automatique", "Auto-Level"},
        {"Luminosité / Contraste...", "Brightness / Contrast..."},
        {"Teinte / Saturation...", "Hue / Saturation..."},
        {"Niveaux...", "Levels..."},
        {"Courbes...", "Curves..."},
        {"Inverser les couleurs", "Invert Colors"},
        {"Sépia...", "Sepia..."},
        {"Sépia", "Sepia"},
        {"Postériser...", "Posterize..."},
        {"Postériser", "Posterize"},
        {"Noir et blanc...", "Black and White..."},
        {"Balance des couleurs...", "Color Balance..."},

        // ---- Effects: categories ----
        {"Artistique", "Artistic"},
        {"Flous", "Blurs"},
        {"Déformation", "Distort"},
        {"Bruit", "Noise"},
        {"Photo", "Photo"},
        {"Rendu", "Render"},
        {"Styliser", "Stylize"},
        {"Répéter le dernier effet", "Repeat last effect"},
        {"Répéter : ", "Repeat: "},
        {"Effets rapides", "Quick effects"},

        // ---- Effects: items ----
        {"Croquis à l'encre...", "Ink Sketch..."},
        {"Croquis à l'encre", "Ink Sketch"},
        {"Peinture à l'huile...", "Oil Painting..."},
        {"Croquis au crayon...", "Pencil Sketch..."},
        {"Fragment...", "Fragment..."},
        {"Flou gaussien...", "Gaussian Blur..."},
        {"Flou directionnel...", "Motion Blur..."},
        {"Flou radial...", "Radial Blur..."},
        {"Flou de surface...", "Surface Blur..."},
        {"Flou...", "Unfocus..."},
        {"Flou de zoom...", "Zoom Blur..."},
        {"Bombement...", "Bulge..."},
        {"Cristalliser...", "Crystalize..."},
        {"Bosselure...", "Dents..."},
        {"Verre givré...", "Frosted Glass..."},
        {"Verre givré", "Frosted Glass"},
        {"Pixéliser...", "Pixelate..."},
        {"Inversion polaire...", "Polar Inversion..."},
        {"Réflexion en mosaïque...", "Tile Reflection..."},
        {"Réflexion en mosaïque", "Tile Reflection"},
        {"Torsion...", "Twist..."},
        {"Ajouter du bruit...", "Add Noise..."},
        {"Médiane...", "Median..."},
        {"Quantifier...", "Reduce Palette..."},
        {"Réduire le bruit...", "Reduce Noise..."},
        {"Réduire le bruit", "Reduce Noise"},
        {"Lueur...", "Glow..."},
        {"Suppression yeux rouges...", "Red Eye Removal..."},
        {"Netteté...", "Sharpen..."},
        {"Adoucir le portrait...", "Soften Portrait..."},
        {"Vignette...", "Vignette..."},
        {"Nuages...", "Clouds..."},
        {"Fractale Julia...", "Julia Fractal..."},
        {"Fractale Mandelbrot...", "Mandelbrot Fractal..."},
        {"Turbulence...", "Turbulence..."},
        {"Détection de contours", "Edge Detect"},
        {"Estampage...", "Emboss..."},
        {"Morphologie...", "Morphology..."},
        {"Contour...", "Outline..."},
        {"Relief...", "Relief..."},

        // ---- Tools ----
        {"Sélection rectangle", "Rectangle Select"},
        {"Lasso de sélection", "Lasso Select"},
        {"Sélection ellipse", "Ellipse Select"},
        {"Baguette magique", "Magic Wand"},
        {"Déplacer les pixels sélectionnés", "Move Selected Pixels"},
        {"Déplacer la sélection", "Move Selection"},
        {"Zoom / Loupe", "Zoom"},
        {"Zoom", "Zoom"},
        {"Se déplacer dans l'image", "Pan"},
        {"Panoramique", "Pan"},
        {"Pot de peinture", "Paint Bucket"},
        {"Dégradé", "Gradient"},
        {"Pinceau", "Paintbrush"},
        {"Gomme", "Eraser"},
        {"Crayon", "Pencil"},
        {"Sélecteur de couleur", "Color Picker"},
        {"Tampon de clonage", "Clone Stamp"},
        {"Recoloriage", "Recolor"},
        {"Texte", "Text"},
        {"Ligne / Courbe", "Line / Curve"},
        {"Formes", "Shapes"},

        // ---- Tool options bar ----
        {"Largeur :", "Width:"},
        {"Dureté :", "Hardness:"},
        {"Espacement :", "Spacing:"},
        {"Tolérance :", "Tolerance:"},
        {"Opacité :", "Opacity:"},
        {"Remplissage :", "Fill:"},
        {"Mode :", "Mode:"},
        {"Taille :", "Size:"},
        {"Police", "Font"},
        {"Gras", "Bold"},
        {"Italique", "Italic"},
        {"Souligné", "Underline"},
        {"Anticrénelage", "Antialiasing"},
        {"Contour", "Outline"},
        {"Rempli", "Filled"},
        {"Contour + rempli", "Outline + Filled"},
        {"Largeur du pinceau ( [ et ] pour ajuster )", "Brush width ( [ and ] to adjust )"},

        // ---- Toolbar tooltips ----
        {"Nouveau", "New"},
        {"Ouvrir", "Open"},
        {"Enregistrer", "Save"},
        {"Imprimer", "Print"},
        {"Couper", "Cut"},
        {"Copier", "Copy"},
        {"Coller", "Paste"},
        {"Désélectionner", "Deselect"},
        {"Annuler", "Undo"},
        {"Rétablir", "Redo"},
        {"Grille de pixels", "Pixel Grid"},
        {"Règles", "Rulers"},
        {"Supprimer", "Delete"},

        // ---- Utility windows ----
        {"Outils", "Tools"},
        {"Historique", "History"},
        {"Calques", "Layers"},
        {"Couleurs", "Colors"},
        {"Paramètres", "Settings"},
        {"Aide", "Help"},
        {"À &propos de paint.software", "&About paint.software"},
        {"À propos de paint.software", "About paint.software"},

        // ---- Colors panel ----
        {"Primaire", "Primary"},
        {"Secondaire", "Secondary"},
        {"<< Moins", "<< Less"},
        {"Plus >>", "More >>"},
        {"Hexa :", "Hex:"},

        // ---- Status bar / help texts ----
        {"Prêt.", "Ready."},
        {"Clic gauche pour dessiner avec la couleur primaire, clic droit avec la couleur secondaire.",
         "Left click to draw with the primary color, right click for the secondary color."},
        {"Clic gauche pour effacer vers transparent. Clic droit vers la couleur secondaire.",
         "Left click to erase to transparent. Right click to the secondary color."},
        {"Clic gauche pour remplir une zone avec la couleur primaire.",
         "Left click to fill an area with the primary color."},
        {"Clic gauche pour définir la couleur primaire. Clic droit pour la secondaire.",
         "Left click to set the primary color. Right click for the secondary."},
        {"Cliquer-glisser pour tracer une sélection. Maintenir Maj pour contraindre.",
         "Click and drag to draw a selection. Hold Shift to constrain."},
        {"Cliquer pour sélectionner une zone de couleur similaire.",
         "Click to select an area of similar color."},
        {"Cliquer-glisser pour déplacer les pixels sélectionnés ou le calque.",
         "Click and drag to move the selected pixels or the layer."},
        {"Cliquer-glisser pour déplacer le contour de sélection sans déplacer les pixels.",
         "Click and drag to move the selection outline without moving pixels."},
        {"Clic gauche pour zoomer. Clic droit pour dézoomer.",
         "Left click to zoom in. Right click to zoom out."},
        {"Cliquer-glisser pour se déplacer dans l'image.",
         "Click and drag to pan around the image."},
        {"Cliquer-glisser pour tracer une sélection à main levée.",
         "Click and drag to draw a freehand selection."},
        {"Cliquer pour placer du texte sur le canevas.",
         "Click to place text on the canvas."},
        {"Cliquer-glisser pour tracer une ligne. Maintenir Maj pour contraindre l'angle.",
         "Click and drag to draw a line. Hold Shift to constrain the angle."},
        {"Cliquer-glisser pour tracer une forme. Maintenir Maj pour contraindre les proportions.",
         "Click and drag to draw a shape. Hold Shift to constrain proportions."},
        {"Cliquer-glisser pour tracer un dégradé de couleur.",
         "Click and drag to draw a color gradient."},
        {"Ctrl + clic pour définir la source. Clic gauche pour peindre depuis la source.",
         "Ctrl + click to set the source. Left click to paint from the source."},
        {"Clic gauche pour dessiner en 1px avec la couleur primaire. Clic droit pour la secondaire.",
         "Left click to draw 1px with the primary color. Right click for the secondary."},
        {"Clic gauche pour remplacer la couleur secondaire par la primaire.",
         "Left click to replace the secondary color with the primary."},

        // ---- Dialogs ----
        {"Erreur", "Error"},
        {"Enregistrer les modifications", "Save changes"},
        {"Le document a été modifié.\nVoulez-vous enregistrer les modifications ?",
         "The document has been modified.\nDo you want to save your changes?"},
        {"Impossible d'ouvrir le fichier image.", "Could not open the image file."},
        {"Impossible d'enregistrer le fichier.", "Could not save the file."},
        {"Ouvrir une image", "Open an image"},
        {"Enregistrer l'image", "Save image"},
        {"Imprimer l'image", "Print image"},
        {"Annuler la sélection", "Cancel"},
        {"Courbes", "Curves"},
        {"Glissez les points. Clic pour ajouter, clic droit pour supprimer.",
         "Drag the points. Click to add, right-click to remove."},
        {"Réinitialiser", "Reset"},

        // ---- Settings dialog ----
        {"Interface utilisateur", "User Interface"},
        {"Canevas", "Canvas"},
        {"Langue :", "Language:"},
        {"Thème :", "Color Scheme:"},
        {"Clair", "Light"},
        {"Sombre", "Dark"},
        {"Défaut (système)", "Default (system)"},
        {"Français", "French"},
        {"Anglais", "English"},
        {"Luminosité du damier de transparence :", "Transparency checkerboard brightness:"},
        {"Fermer", "Close"},
        {"Le changement de langue est appliqué immédiatement.",
         "The language change is applied immediately."},
        {"Options...", "Settings..."},

        // ---- Utility-icon tooltips (menu bar, right side) ----
        {"Affiche ou masque la palette d'outils (pinceau, sélection, formes...).",
         "Shows or hides the Tools palette (brush, selection, shapes...)."},
        {"Affiche ou masque l'historique : chaque action est listée, cliquez pour y revenir.",
         "Shows or hides History: every action is listed; click one to go back to it."},
        {"Affiche ou masque les calques : ajouter, supprimer, réordonner, opacité et fusion.",
         "Shows or hides Layers: add, delete, reorder, opacity and blend mode."},
        {"Affiche ou masque les couleurs : couleur primaire/secondaire, roue, RVB/TSV et palette.",
         "Shows or hides Colors: primary/secondary color, wheel, RGB/HSV and palette."},
        {"Ouvre les paramètres : langue (français / anglais), thème clair ou sombre, et canevas.",
         "Opens Settings: language (French / English), light or dark color scheme, and canvas."},
        {"Raccourcis clavier, documentation et informations sur la version.",
         "Keyboard shortcuts, documentation and version information."},
        {"replacer la fenêtre", "reset the window position"},
        {"Ctrl+Maj+", "Ctrl+Shift+"},

        // ---- Shortcuts help ----
        {"Raccourcis clavier", "Keyboard shortcuts"},
        {"Sélections (cycle)", "Selection tools (cycle)"},
        {"Déplacement (cycle)", "Move tools (cycle)"},
        {"Ligne / Formes (cycle)", "Line / Shapes (cycle)"},
        {"Outils directs", "Direct tool keys"},
        {"Taille du pinceau", "Brush size"},
        {"Échanger / basculer les couleurs", "Swap / toggle colors"},
        {"Fenêtres Outils / Historique / Calques / Couleurs",
         "Tools / History / Layers / Colors windows"},
        {"Replacer une fenêtre", "Reset a window's position"},
        {"Annuler / Rétablir", "Undo / Redo"},
        {"Clic : couleur primaire · Clic droit : couleur secondaire",
         "Click: primary color · Right-click: secondary color"},

        // ---- Tool variant combo ----
        {"Type :", "Type:"},
        {"Forme :", "Shape:"},
        {"Style :", "Style:"},
        {"Rectangle", "Rectangle"},
        {"Ellipse", "Ellipse"},
        {"Rectangle arrondi", "Rounded Rectangle"},
        {"Triangle", "Triangle"},
        {"Losange", "Diamond"},
        {"Pentagone", "Pentagon"},
        {"Hexagone", "Hexagon"},
        {"Étoile", "Star"},
        {"Linéaire", "Linear"},
        {"Radial", "Radial"},
        {"Conique", "Conical"},
        {"Trait plein", "Solid"},
        {"Flèche", "Arrow"},
        {"Pointillés", "Dashed"},
        {"Contigu", "Contiguous"},
        {"Global", "Global"},

        // ---- Tool descriptions (tooltips) ----
        {"Sélectionne une zone rectangulaire à modifier.",
         "Selects a rectangular area to edit."},
        {"Sélectionne une zone ovale à modifier.",
         "Selects an oval area to edit."},
        {"Sélectionne une zone à main levée en la traçant.",
         "Selects a freehand area by drawing around it."},
        {"Sélectionne automatiquement une plage de couleurs similaires.",
         "Automatically selects a range of similar colors."},
        {"Déplace les pixels de la sélection (ou tout le calque).",
         "Moves the selected pixels (or the whole layer)."},
        {"Déplace le contour de la sélection sans déplacer les pixels.",
         "Moves the selection outline without moving the pixels."},
        {"Agrandit (clic gauche) ou réduit (clic droit) l'affichage.",
         "Zooms in (left click) or out (right click)."},
        {"Déplace la vue dans l'image en cliquant-glissant (main).",
         "Pans the view around the image by dragging (hand)."},
        {"Remplit une zone de couleur uniforme avec la couleur active.",
         "Fills a uniformly-colored area with the active color."},
        {"Trace un dégradé entre la couleur primaire et secondaire.",
         "Draws a gradient between the primary and secondary color."},
        {"Peint des traits doux avec la couleur active.",
         "Paints soft strokes with the active color."},
        {"Efface vers la transparence (ou la couleur secondaire).",
         "Erases to transparency (or the secondary color)."},
        {"Dessine des pixels nets, sans anticrénelage, en 1 px.",
         "Draws crisp 1-px pixels with no antialiasing."},
        {"Prélève une couleur de l'image comme couleur active.",
         "Picks a color from the image as the active color."},
        {"Duplique une partie de l'image (Ctrl+clic définit la source).",
         "Clones part of the image (Ctrl+click sets the source)."},
        {"Remplace une couleur par la couleur active là où l'on peint.",
         "Replaces a color with the active color where you paint."},
        {"Ajoute du texte sur l'image.", "Adds text on the image."},
        {"Trace des lignes et des courbes.", "Draws lines and curves."},
        {"Dessine des formes (rectangle, ellipse, étoile...).",
         "Draws shapes (rectangle, ellipse, star...)."},

        // ---- New adjustments / effects / edit items ----
        {"Copier &fusionné", "Copy &Merged"},
        {"Coller dans un &nouveau calque", "Paste into &New Layer"},
        {"Zoom sur la &sélection", "Zoom to &Selection"},
        {"Exposition...", "Exposure..."},
        {"Exposition", "Exposure"},
        {"Exposition (-100 à 100) :", "Exposure (-100 to 100):"},
        {"Hautes / Basses lumières...", "Highlights / Shadows..."},
        {"Hautes / Basses lumières", "Highlights / Shadows"},
        {"Hautes lumières (-100 à 100) :", "Highlights (-100 to 100):"},
        {"Basses lumières (-100 à 100) :", "Shadows (-100 to 100):"},
        {"Température / Teinte...", "Temperature / Tint..."},
        {"Température / Teinte", "Temperature / Tint"},
        {"Température (-100 froid à 100 chaud) :", "Temperature (-100 cool to 100 warm):"},
        {"Teinte (-100 vert à 100 magenta) :", "Tint (-100 green to 100 magenta):"},
        {"Inverser l'alpha", "Invert Alpha"},
        {"Objet", "Object"},
        {"Ombre portée...", "Drop Shadow..."},
        {"Ombre portée", "Drop Shadow"},
        {"Rayon du flou :", "Blur radius:"},
        {"Décalage X :", "Offset X:"},
        {"Décalage Y :", "Offset Y:"},
        {"Opacité (0-100) :", "Opacity (0-100):"},
        {"Propriétés du calque", "Layer Properties"},
        {"&Unités", "&Units"},
        {"Pixels", "Pixels"},
        {"Pouces", "Inches"},
        {"Centimètres", "Centimeters"},
        {"Récupération", "Recovery"},
        {"un document sans titre", "an untitled document"},

        // ---- Layer context menu / smart merge ----
        {"Renommer...", "Rename..."},
        {"Opacité...", "Opacity..."},
        {"Renommer le calque", "Rename Layer"},
        {"Nom du calque :", "Layer name:"},
        {"Opacité du calque", "Layer Opacity"},
        {"Opacité (0-100 %) :", "Opacity (0-100%):"},
        {"Fusionner les calques", "Merge Layers"},
        {"Comment fusionner les deux calques ?", "How should the two layers be merged?"},
        {"Superposer (normal)", "Combine (normal)"},
        {"Garder le calque du dessus", "Keep the top layer"},
        {"Garder le calque du dessous", "Keep the bottom layer"},
        {"Annuler|bouton", "Cancel"},
        {"Le calque du dessous est une couleur unie (arrière-plan).",
         "The bottom layer is a solid colour (background)."},
        {"Le calque du dessus est une couleur unie (arrière-plan).",
         "The top layer is a solid colour (background)."},
        {"Les deux calques contiennent un dessin.",
         "Both layers contain artwork."},

        // ---- Adjustments ----
        {"Luminosité", "Brightness"},
        {"Luminosité / Contraste", "Brightness / Contrast"},
        {"Contraste", "Contrast"},
        {"Balance des couleurs", "Color Balance"},
        {"Cyan / Rouge", "Cyan / Red"},
        {"Magenta / Vert", "Magenta / Green"},
        {"Jaune / Bleu", "Yellow / Blue"},
        {"Niveaux", "Levels"},
        {"Noir d'entrée", "Input black"},
        {"Blanc d'entrée", "Input white"},
        {"Gamma", "Gamma"},
        {"Hautes lumières", "Highlights"},
        {"Basses lumières", "Shadows"},
        {"Noir et blanc", "Black and White"},
        {"Teinte (vert → magenta)", "Tint (green → magenta)"},
        {"Température (froid → chaud)", "Temperature (cool → warm)"},

        // ---- Effects ----
        {"Flou gaussien", "Gaussian Blur"},
        {"Netteté", "Sharpen"},
        {"Pixéliser", "Pixelate"},
        {"Rayon", "Radius"},
        {"Quantité", "Amount"},
        {"Intensité", "Intensity"},
        {"Taille de cellule", "Cell size"},

        // ---- Image / selection commands ----
        {"Retourner horizontalement", "Flip Horizontal"},
        {"Retourner verticalement", "Flip Vertical"},
        {"Inverser la sélection", "Invert Selection"},
        {"Aplatir les calques ?", "Flatten layers?"},
        {"Garder les deux dessins", "Keep both drawings"},

        // ---- View ----
        {"Zoom avant", "Zoom In"},
        {"Zoom arrière", "Zoom Out"},
        {"Barre d'outils", "Toolbar"},

        // ---- Plugins ----
        {"Plugins", "Plugins"},
        {"Aucun plugin chargé", "No plugin loaded"},
        {"Ouvrir le dossier des plugins…", "Open the plugins folder…"},

        // ---- Misc ----
        {"Sans titre", "Untitled"},
        {"Version", "Version"},
        {"édition Linux", "Linux edition"},

        // ---- Resize / Canvas size / Hue-Saturation dialogs ----
        {"Redimensionner", "Resize"},
        {"Taille en pixels", "Pixel size"},
        {"Par taille &absolue :", "By &absolute size:"},
        {"Par &pourcentage :", "By &percentage:"},
        {"Taille d'impression", "Print size"},
        {"&Conserver les proportions", "&Maintain aspect ratio"},
        {"Rééchantillonnage :", "Resampling:"},
        {"Meilleure qualité", "Best Quality"},
        {"Bicubique", "Bicubic"},
        {"Bilinéaire", "Bilinear"},
        {"Plus proche voisin", "Nearest Neighbor"},
        {"Suréchantillonnage", "Super Sampling"},
        {"Nouvelle taille : %1 x %2 pixels", "New size: %1 x %2 pixels"},
        {"Taille du canevas", "Canvas Size"},
        {"Nouvelle taille", "New size"},
        {"Ancrage", "Anchor"},
        {"Le nouvel espace sera rempli avec la couleur secondaire.",
         "The new space will be filled with the secondary color."},
        {"Teinte / Saturation", "Hue / Saturation"},
        {"Teinte", "Hue"},
        // HSL lightness, not the Brightness/Contrast adjustment: same French word.
        {"Luminosité|hsl", "Lightness"},
        {"Mode de fusion", "Blend mode"},
        {"Taille de l'image", "Image size"},
        {"Hauteur :", "Height:"},
        {"Conserver le ratio", "Maintain aspect ratio"},
        {"%1\n%2 × %3\n(clic milieu pour fermer)", "%1\n%2 × %3\n(middle-click to close)"},
        {"Astuce : F5–F8 affichent/masquent les fenêtres Outils, Historique, ",
         "Tip: F5–F8 show/hide the Tools, History, "},

        // ---- Effect dialogs: titles ----
        {"Flou", "Blur"},
        {"Flou directionnel", "Motion Blur"},
        {"Flou radial", "Radial Blur"},
        {"Flou de zoom", "Zoom Blur"},
        {"Flou de surface", "Surface Blur"},
        {"Ajouter du bruit", "Add Noise"},
        {"Médiane", "Median"},
        {"Relief", "Relief"},
        {"Bombement", "Bulge"},
        {"Torsion", "Twist"},
        {"Bosselure", "Dents"},
        {"Cristalliser", "Crystalize"},
        {"Fragment", "Fragment"},
        {"Nuages", "Clouds"},
        {"Turbulence", "Turbulence"},
        {"Inversion polaire", "Polar Inversion"},
        {"Quantifier", "Quantize"},
        {"Lueur", "Glow"},
        {"Vignette", "Vignette"},
        {"Morphologie", "Morphology"},
        {"Peinture à l'huile", "Oil Painting"},
        {"Croquis au crayon", "Pencil Sketch"},
        {"Adoucir le portrait", "Soften Portrait"},
        {"Suppression yeux rouges", "Red Eye Removal"},

        // ---- Effect dialogs: labels ----
        {"Rayon :", "Radius:"},
        {"Quantité :", "Amount:"},
        {"Intensité :", "Intensity:"},
        {"Angle :", "Angle:"},
        {"Distance :", "Distance:"},
        {"Échelle :", "Scale:"},
        {"Force :", "Strength:"},
        {"Douceur :", "Softness:"},
        {"Épaisseur :", "Thickness:"},
        {"Rugosité :", "Roughness:"},
        {"Réfraction :", "Refraction:"},
        {"Rotation :", "Rotation:"},
        {"Saturation :", "Saturation:"},
        {"Seuil :", "Threshold:"},
        {"Couleurs :", "Colors:"},
        {"Fragments :", "Fragments:"},
        {"Luminosité :", "Brightness:"},
        {"Chaleur :", "Warmth:"},
        {"Contour encre :", "Ink outline:"},
        {"Taille de cellule :", "Cell size:"},
        {"Taille de tuile :", "Tile size:"},
        {"Taille du pinceau :", "Brush size:"},
        {"Taille de mine :", "Pencil tip size:"},
        {"Quantité (-100 à 100) :", "Amount (-100 to 100):"},
        {"Quantité (-360 à 360) :", "Amount (-360 to 360):"},

        // ---- New document / layer properties dialogs ----
        {"Préréglage :", "Preset:"},
        {"Personnalisé", "Custom"},
        {"Résolution :", "Resolution:"},
        {"Arrière-plan :", "Background:"},
        {"Couleur d'arrière-plan", "Background color"},
        {"Général", "General"},
        {"Opacité", "Opacity"},

        // ---- Panels / plugins ----
        {"Annuler (Ctrl+Z)", "Undo (Ctrl+Z)"},
        {"Rétablir (Ctrl+Y)", "Redo (Ctrl+Y)"},
        {"Réinitialiser les couleurs par défaut", "Reset to the default colors"},
        {"Permuter les couleurs", "Swap colors"},
        {"Paramètre %1", "Parameter %1"},

        // ---- Messages ----
        {"Impossible de charger le fichier image.", "Could not load the image file."},
        {"Une sauvegarde automatique a été trouvée (%1, %2).\nVoulez-vous la récupérer ?", "An autosave was found (%1, %2).\nDo you want to recover it?"},
        {"Ce format ne conserve pas les calques : l'image sera aplatie.\nUtilisez le format .psw pour garder vos calques.\n\nContinuer ?", "This format does not keep layers: the image will be flattened.\nUse the .psw format to keep your layers.\n\nContinue?"},
    };
    return m;
}

} // namespace

namespace I18n {

void setLanguage(Lang lang) { g_lang = lang; }
Lang language() { return g_lang; }

void applyQtTranslations() {
    // Qt draws the standard buttons (OK / Cancel / Yes / No / Save) from its own
    // catalogue, which our table never sees. English is Qt's built-in default, so
    // only French needs a catalogue loaded — and it is removed again on the way
    // back so a switch to English takes effect without a restart.
    static QTranslator *qtTranslator = nullptr;
    if (qtTranslator) {
        QCoreApplication::removeTranslator(qtTranslator);
        delete qtTranslator;
        qtTranslator = nullptr;
    }
    if (g_lang != Lang::French) return;

    auto *t = new QTranslator;
    // qtbase_fr.qm ships in a separate package; if it is missing, Qt simply keeps
    // its English strings rather than failing.
    if (t->load(QStringLiteral("qtbase_fr"),
                QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(t);
        qtTranslator = t;
    } else {
        delete t;
    }
}

void loadFromSettings() {
    QSettings s("PaintDali", "PaintDali");
    // English by default: the app ships worldwide, so a first run with no saved
    // choice should land on the more widely understood language. Picking a
    // language in Settings writes the key, and that choice then wins forever.
    const QString v = s.value("ui/language", "en").toString();
    g_lang = (v == "fr") ? Lang::French : Lang::English;
}

void saveToSettings() {
    QSettings s("PaintDali", "PaintDali");
    s.setValue("ui/language", g_lang == Lang::English ? "en" : "fr");
}

QString t(const QString &french) {
    // A "text|context" key disambiguates one French string that needs two
    // different English words — "Annuler" is Undo in the Edit menu but Cancel on
    // a dialog button, and a single table can only hold one of them. The context
    // suffix is never displayed.
    const int bar = french.indexOf(QLatin1Char('|'));
    const QString plain = (bar < 0) ? french : french.left(bar);
    if (g_lang == Lang::French) return plain;
    auto it = table().constFind(french);
    return (it != table().constEnd()) ? it.value() : plain;
}

} // namespace I18n
