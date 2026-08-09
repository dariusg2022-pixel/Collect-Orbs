#include "raylib.h"
#include <random>
#include <string>
#include <vector>

using namespace std;

Color colors[] = { RED, BLUE, GREEN, SKYBLUE, YELLOW, PURPLE };

struct { // for the essential game variables
    bool playing = true;
    bool shopping = false;
    bool issettings = false;
    bool iscredits = false;
    bool inmenu = true;
    const char* gamename = "Collect The Orbs";
    int screenwidth = 800;
    int screenheight = 500;
} game_main;

struct { // stores variables about the positions and sizes of the boundaries.
    Rectangle top = Rectangle{ 0, 50, float(game_main.screenwidth), 15 };
    Rectangle bottom = Rectangle{ 0, float(game_main.screenheight) - 15, float(game_main.screenwidth), 15 };
    Rectangle left = Rectangle{ 0, 50, 15, float(game_main.screenheight) };
    Rectangle right = Rectangle{ float(game_main.screenwidth) - 15, 50, 15, float(game_main.screenheight) };
} borders;

struct { // stores variables and player position and size
    float playerSpeedMult = 1; // multiplier
    float playerSpeedDef = 150; // default
    float playerSpeed = playerSpeedDef * playerSpeedMult; // player speed
    Color playercolor = colors[rand() % 6];
    int score = 0;
    Rectangle player = Rectangle{ 0, 0, 25, 25 };
} player_char;

struct { // stores variables for orbs - colors, positions and sizes of all robs.
    int OrbNum = 1;
    bool orbrespawn = false;
    int orbvalinc = 0;
    Color orbColor[100] = { RED };
    Rectangle orb_rects[100] = { Rectangle {1, 1, 10, 10} }; // square outside circle is 2r | x and y are half of width
} orb;

struct { // stores variables for the buttons
    bool shophover = false;
    bool settingshover = false;
} button;

struct { // stores variables and the file paths to all the different sounds in the game.
    Sound collectsound = {};
    Sound buttonhover = {};
    vector<string> BackgroundSongs;
    Music currentsong;
    int currentsongIndex = -1;
    float musicvolume = 1.0f;
} sounds;

/// <summary>
///  Relatively simple movement, that uses frametime to get a consistent speed and it normalizes values so that diagonals aren't faster than normal movement on X and Y.
/// </summary>
/// <param name="position">Player's position.</param>
/// <param name="speed"></param>
void playermovement(Vector2& position, float speed)
{
    float frametime = GetFrameTime(); // from my understanding, this decimal numbers gets lower with higher fps so speed is consistent

    Vector2 dir = { 0, 0 };

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) dir.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dir.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) dir.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1;

    float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (length > 0)
    {
        dir.x /= length;
        dir.y /= length;
    }

    position.x += dir.x * speed * frametime;
    position.y += dir.y * speed * frametime;
}

/// <summary>
///  Function that checks if a rectangle was clicked by the mouse using a specific mouse button
/// </summary>
/// <param name="rect">The rect to check</param>
/// <param name="mousebutton">The specific mouse button to check for</param>
/// <returns></returns>
bool IsRectClick(Rectangle rect, int mousebutton)
{
    return IsMouseButtonPressed(mousebutton) && CheckCollisionPointRec(GetMousePosition(), rect);
}

/// <summary>
///  Simple function that plays a sound if the mouse enters or leaves a rectangle.
/// </summary>
/// <param name="rect"></param>
/// <param name="washovering">Bool to check if it was hovering over the rectangle (It is important or it will constantly play the sound)</param>
/// <param name="buttonhover">The sound for hovering over the rectangle.</param>
/// <param name="buttonleave">The sound for leaving the rectangle.</param>
void MouseHoverOver(Rectangle rect, bool& washovering, Sound buttonenter, Sound buttonleave)
{
    bool hovering = CheckCollisionPointRec(GetMousePosition(), rect);

    if (hovering && !washovering)
    {
        PlaySound(buttonenter);
    }

    if (!hovering && washovering)
    {
        PlaySound(buttonleave);
    }

    washovering = hovering;
}

/// <summary>
///  Function that checks individual red, green, blue and alpha components.
/// </summary>
/// <param name="a">First color.</param>
/// <param name="b">Second color.</param>
/// <returns></returns>
bool ColorEquals(Color a, Color b) // orb collection helper
{
    return a.r == b.r &&
        a.g == b.g &&
        a.b == b.b &&
        a.a == b.a;
}

/// <summary>
///  Checks the value of an orb - calculated via color and increase in price
/// </summary>
/// <param name="color"></param>
/// <param name="increase"></param>
/// <returns></returns>
int ValueCheck(Color color, int increase)
{ // helper for orb collection btw
    if (ColorEquals(color, RED)) return 1 + increase;
    if (ColorEquals(color, BLUE)) return 2 + increase;
    if (ColorEquals(color, GREEN)) return 3 + increase;

    return 0;
}

/// <summary>
///  Simple script that draws a customizable text button.
/// </summary>
/// <param name="posx">Position X of the button</param>
/// <param name="posy">Position Y of the button</param>
/// <param name="width">Width of the button</param>
/// <param name="height">Height of the button</param>
/// <param name="Background">Color of the background</param>
/// <param name="Outline">Outline color</param>
/// <param name="OutlineThickness">Thickness of the outline</param>
/// <param name="ButtonText">The actual text on the button</param>
/// <param name="posxT">Position X of the text</param>
/// <param name="posyT">Position Y of the text</param>
/// <param name="fontS">Size of the font</param>
/// <param name="TColor">Color of the text</param>
void DrawButton(int posx, int posy, int width, int height, Color Background, Color Outline, int OutlineThickness, string ButtonText, int posxT, int posyT, int fontS, Color TColor)
{ // Misc. function because i've been drawing a lot of buttons
    DrawRectangle(posx, posy, width, height, Background); // background
    DrawRectangleLinesEx({ float(posx), float(posy), float(width), float(height) }, OutlineThickness, Outline); // outline
    DrawText(ButtonText.c_str(), posxT, posyT, fontS, TColor); // text
}

/// <summary>
///  Function that spawns in orbs.
/// </summary>
/// <param name="colorlist">The list of colors orbs can be. </param>
/// <param name="PlayArea">The area where orbs can spawn.</param>
void orb_spawn(Color colorlist[], Rectangle PlayArea)
{
    int colorsrand = 3;

    for (int i = 0; i < orb.OrbNum; i++)
    {
        orb.orb_rects[i].width = 10;
        orb.orb_rects[i].height = 10;
        orb.orb_rects[i].x = PlayArea.x + rand() % int(PlayArea.width - orb.orb_rects[i].width);
        orb.orb_rects[i].y = PlayArea.y + rand() % int(PlayArea.height - orb.orb_rects[i].height);
        orb.orbColor[i] = colorlist[rand() % colorsrand];
    }
}

/// <summary>
///  Function containing the logic for collecting the orbs.
/// </summary>
/// <param name="Hitbox1">Player's hitbox.</param>
/// <param name="Hitbox2">Orbs hitbox LIST.</param>
/// <param name="h2len">The length of the orb hitbox list.</param>
/// <param name="PlayArea">Area where orbs can spawn.</param>
/// <param name="Hitbox2Color">The LIST of the color of the orbs.</param>
/// <param name="colorlist">The LIST of possible colors for orbs.</param>
int orb_collection(Rectangle& Hitbox1, Rectangle Hitbox2[], int h2len, Rectangle PlayArea, Color Hitbox2Color[], Color colorlist[])
{
    int colorsrand = 3; // the three colors it can be
    int value;

    for (int i = 0; i < h2len; i++)
    {
        if (CheckCollisionRecs(Hitbox1, Hitbox2[i]))
        {
            value = ValueCheck(Hitbox2Color[i], orb.orbvalinc);

            SetSoundPitch(sounds.collectsound, 0.8f + float(rand()) / float(RAND_MAX) * (1.3f - 0.8f)); // pitch variation
            PlaySound(sounds.collectsound); // plays the .mp3 file for the sound

            Hitbox2[i].x = PlayArea.x + rand() % int(PlayArea.width - Hitbox2[i].width);
            Hitbox2[i].y = PlayArea.y + rand() % int(PlayArea.height - Hitbox2[i].height);
            Hitbox2Color[i] = colorlist[rand() % colorsrand];

            return value;
        }
    }

    return 0;
}

/// <summary>
/// Easter egg - used for playing the meows of my mascot, Ralph.
/// </summary>
void ralphmews()
{
    static bool initialized = false;
    static Sound RalphList[6];
    if (!initialized)
    {
        RalphList[0] = LoadSound("assets/Sounds/SFX/ralph1.mp3");
        RalphList[1] = LoadSound("assets/Sounds/SFX/ralph2.mp3");
        RalphList[2] = LoadSound("assets/Sounds/SFX/ralph3.mp3");
        RalphList[3] = LoadSound("assets/Sounds/SFX/ralph4.mp3");
        RalphList[4] = LoadSound("assets/Sounds/SFX/ralph5.wav");
        RalphList[5] = LoadSound("assets/Sounds/SFX/ralph6.wav");

        initialized = true;
    }

    PlaySound(RalphList[rand() % 6]);
}

/// <summary>
///  This function loads the entire shop, including logic.
/// </summary>
/// <param name="Placeholder"> Placeholder at the bottom of the page.</param>
void shop(Texture2D Placeholder)
{
    // local variables
    static float pricemult = 0.5;
    static int defaultprice = 2;

    // drawing
    DrawRectangle(275, 75, 250, 400, GRAY); // background
    DrawRectangleLinesEx({ 275, 75, 250, 400 }, 5, LIGHTGRAY); // outline

    DrawText("Speed", 285, 85, 32, WHITE);
    Rectangle IncSpeed = Rectangle{ 395,86,25,25 };
    DrawRectangle(IncSpeed.x, IncSpeed.y, IncSpeed.width, IncSpeed.height, GREEN); // increase button
    DrawText(TextFormat("x%.2f", player_char.playerSpeedMult), 430, 85, 32, WHITE); // .2 is how many digits after decimal
    if (player_char.playerSpeedMult < float(4)) DrawText(TextFormat("Price: $%.1f", round(defaultprice * pricemult)), 285, 120, 16, WHITE); // price tag + maxed
    else DrawText("MAXED", 285, 120, 16, WHITE);

    DrawText("Orbs", 285, 150, 32, WHITE);
    Rectangle IncOrb = Rectangle{ 375,156,25,25 };
    DrawRectangle(IncOrb.x, IncOrb.y, IncOrb.width, IncOrb.height, GREEN); // increase button
    DrawText(TextFormat("%d", orb.OrbNum), 410, 155, 32, WHITE); // no decimal
    if (orb.OrbNum < 12) DrawText(TextFormat("Price: $%.1f", round((defaultprice + 1) * pricemult)), 285, 190, 16, WHITE); // price tag + maxed
    else DrawText("MAXED", 285, 190, 16, WHITE);

    DrawText("Orb Price", 285, 220, 32, WHITE);
    Rectangle IncOrbPrice = Rectangle{ 450,226,25,25 };
    DrawRectangle(IncOrbPrice.x, IncOrbPrice.y, IncOrbPrice.width, IncOrbPrice.height, GREEN); // increase button
    DrawText(TextFormat("%d", orb.orbvalinc), 285, 255, 32, WHITE); // no decimal
    if (orb.orbvalinc < 100) DrawText(TextFormat("Price: $%.1f", round((defaultprice + 2) * pricemult)), 345, 255, 16, WHITE); // price tag + maxed
    else DrawText("MAXED", 345, 255, 16, WHITE);

    // logic (all increase)
    if (IsRectClick(IncSpeed, MOUSE_BUTTON_LEFT) && player_char.score >= round(defaultprice * pricemult) && player_char.playerSpeedMult < float(4)) { // increases speed
        player_char.score -= round(defaultprice * pricemult);
        player_char.playerSpeedMult += 0.25;
        defaultprice += 1;
        pricemult += 0.25;
    }
    if (IsRectClick(IncOrb, MOUSE_BUTTON_LEFT) && player_char.score >= round((defaultprice + 1) * pricemult) && orb.OrbNum < 12) { // inceases orbs on map
        player_char.score -= round((defaultprice + 1) * pricemult);
        orb.OrbNum += 1;
        orb.orbrespawn = true;
        defaultprice += 1;
        pricemult += 0.25;
    }
    if (IsRectClick(IncOrbPrice, MOUSE_BUTTON_LEFT) && player_char.score >= round((defaultprice + 2) * pricemult) && orb.orbvalinc < 100) { // inceases orb value
        player_char.score -= round((defaultprice + 2) * pricemult);
        orb.orbvalinc += 5;
        defaultprice += 5;
        pricemult += 0.25;
    }

    // ralph
    DrawTexture(Placeholder, 295, 365, WHITE);
    if (IsRectClick({ 295, 406, 113, 58 }, MOUSE_BUTTON_LEFT)) ralphmews();
}

/// <summary>
///  This function loads the entire settings menu, including logic.
/// </summary>
/// <param name="texture"> Used as a placeholder at the bottom of the menu.</param>
void settings(Texture2D texture)
{
    static float mastervolume = 1;
    static float sfxvolume = 1;

    // drawing
    DrawRectangle(275, 75, 250, 400, GRAY); // background
    DrawRectangleLinesEx({ 275, 75, 250, 400 }, 5, LIGHTGRAY); // outline

    DrawText("Master Vol. : ", 285, 85, 21, WHITE); // music volume
    DrawText(TextFormat("%.0f", mastervolume * 100), 430, 86, 21, WHITE);
    Rectangle PlusVolumeMas = Rectangle{ 465, 86, 21, 21 };
    DrawButton(PlusVolumeMas.x, PlusVolumeMas.y, PlusVolumeMas.width, PlusVolumeMas.height, GRAY, LIGHTGRAY, 2, "+", 469, 85, 26, WHITE);
    Rectangle MinVolumeMas = Rectangle{ 490, 86, 21, 21 };
    DrawButton(MinVolumeMas.x, MinVolumeMas.y, MinVolumeMas.width, MinVolumeMas.height, GRAY, LIGHTGRAY, 2, "-", 495, 85, 26, WHITE);

    DrawText("Music Volume: ", 285, 115, 21, WHITE); // music volume
    DrawText(TextFormat("%.0f", sounds.musicvolume * 100), 430, 116, 21, WHITE);
    Rectangle PlusVolumeMus = Rectangle{ 465, 116, 21, 21 };
    DrawButton(PlusVolumeMus.x, PlusVolumeMus.y, PlusVolumeMus.width, PlusVolumeMus.height, GRAY, LIGHTGRAY, 2, "+", 469, 115, 26, WHITE);
    Rectangle MinVolumeMus = Rectangle{ 490, 116, 21, 21 };
    DrawButton(MinVolumeMus.x, MinVolumeMus.y, MinVolumeMus.width, MinVolumeMus.height, GRAY, LIGHTGRAY, 2, "-", 495, 115, 26, WHITE);

    DrawText("SFX Volume: ", 285, 145, 21, WHITE); // music volume
    DrawText(TextFormat("%.0f", sfxvolume * 100), 430, 146, 21, WHITE);
    Rectangle PlusVolumeSFX = Rectangle{ 465, 146, 21, 21 };
    DrawButton(PlusVolumeSFX.x, PlusVolumeSFX.y, PlusVolumeSFX.width, PlusVolumeSFX.height, GRAY, LIGHTGRAY, 2, "+", 469, 145, 26, WHITE);
    Rectangle MinVolumeSFX = Rectangle{ 490, 146, 21, 21 };
    DrawButton(MinVolumeSFX.x, MinVolumeSFX.y, MinVolumeSFX.width, MinVolumeSFX.height, GRAY, LIGHTGRAY, 2, "-", 495, 145, 26, WHITE);

    // logic
    if (IsRectClick(PlusVolumeMas, MOUSE_BUTTON_LEFT) && mastervolume < 1) mastervolume += 0.1;

    if (IsRectClick(MinVolumeMas, MOUSE_BUTTON_LEFT) && mastervolume >= 0) mastervolume -= 0.1;

    if (IsRectClick(PlusVolumeMus, MOUSE_BUTTON_LEFT) && sounds.musicvolume < 1) sounds.musicvolume += 0.1;

    if (IsRectClick(MinVolumeMus, MOUSE_BUTTON_LEFT) && sounds.musicvolume >= 0) sounds.musicvolume -= 0.1;

    if (IsRectClick(PlusVolumeSFX, MOUSE_BUTTON_LEFT) && sfxvolume < 1) sfxvolume += 0.1;

    if (IsRectClick(MinVolumeSFX, MOUSE_BUTTON_LEFT) && sfxvolume >= 0) sfxvolume -= 0.1;

    // volume setting
    SetMusicVolume(sounds.currentsong, sounds.musicvolume * mastervolume); // in %, ex: 0.5 is 50%

    // SFX volume setting
    SetSoundVolume(sounds.collectsound, sfxvolume * mastervolume);
    SetSoundVolume(sounds.buttonhover, sfxvolume * mastervolume);

    // ralph
    DrawTexture(texture, 295, 365, WHITE);
    if (IsRectClick({ 295, 406, 113, 58 }, MOUSE_BUTTON_LEFT)) ralphmews();
}

/// <summary>
///  This function loads the entire credits menu.
/// </summary>
/// <param name="texture"> Used as a placeholder at the bottom of the menu.</param>
void credits()
{
    // drawing
    DrawRectangle(275, 75, 250, 400, GRAY); // background
    DrawRectangleLinesEx({ 275, 75, 250, 400 }, 5, LIGHTGRAY); // outline

    // credits
    DrawText("Programmer &\nArtist: Darius", 285, 110, 32, WHITE);
    DrawText("Song Credits: ?", 285, 110, 32, WHITE);
}

/// <summary>
///  This function loads the entire menu, including logic.
/// </summary>
/// <param name="buttonhover"> Sound that plays when hovering over a menu button.</param>
void menu(Sound buttonhover)
{
    // drawing
    DrawText("Collect The Orbs", 180, 50, 50, WHITE); // title
    DrawText("version 1.2", 320, 100, 25, WHITE); // version
    DrawText("by Darius", 680, 470, 23, WHITE);

    DrawButton(280, 180, 200, 50, GRAY, LIGHTGRAY, 5, "PLAY", 332, 187, 40, WHITE); // play button
    static bool playhovering = 0;
    DrawButton(10, 455, 35, 35, GRAY, LIGHTGRAY, 5, "S", 18, 459, 30, WHITE); // settings button
    static bool settingshovering = 0;
    DrawButton(65, 455, 35, 35, GRAY, LIGHTGRAY, 5, "C", 73, 459, 30, WHITE); // play button
    static bool creditshovering = 0;

    // logic
    if (!game_main.issettings && !game_main.iscredits) MouseHoverOver({ 280, 180, 200, 50 }, playhovering, buttonhover, buttonhover);
    if (!game_main.issettings) MouseHoverOver({ 65, 455, 35, 35 }, creditshovering, buttonhover, buttonhover);
    MouseHoverOver({ 10, 455, 35, 35 }, settingshovering, buttonhover, buttonhover);

    if (IsRectClick({ 280, 180, 200, 50 }, MOUSE_BUTTON_LEFT) && !game_main.issettings && !game_main.iscredits) game_main.inmenu = false;

    if (IsRectClick({ 65, 455, 35, 35 }, MOUSE_BUTTON_LEFT) && !game_main.issettings) game_main.iscredits = not game_main.iscredits;

    if (IsRectClick({ 10, 455, 35, 35 }, MOUSE_BUTTON_LEFT)) game_main.issettings = not game_main.issettings;
}

/// <summary>
/// Finds all .mp3 and .wav files in the background music directory.
/// The user only needs to drop music files into this folder.
/// </summary>
void discoverbackgroundsongs()
{
    sounds.BackgroundSongs.clear();

    FilePathList files = LoadDirectoryFiles("assets/Sounds/Songs");

    for (unsigned int i = 0; i < files.count; i++)
    {
        const char* extension = GetFileExtension(files.paths[i]);

        if (extension != nullptr &&
            (TextIsEqual(extension, ".mp3") || TextIsEqual(extension, ".wav")))
        {
            sounds.BackgroundSongs.push_back(files.paths[i]);
        }
    }

    UnloadDirectoryFiles(files);

    // Keep the discovered list deterministic; song selection is still random.
    sort(sounds.BackgroundSongs.begin(), sounds.BackgroundSongs.end());
}

/// <summary>
/// Loads a random background song, avoiding the same song twice in a row
/// when there is more than one song available.
/// </summary>
void musicplayerchooser()
{
    if (sounds.BackgroundSongs.empty())
        return;

    int chosenSong = rand() % sounds.BackgroundSongs.size();

    if (sounds.BackgroundSongs.size() > 1)
    {
        while (chosenSong == sounds.currentsongIndex)
            chosenSong = rand() % sounds.BackgroundSongs.size();
    }

    sounds.currentsongIndex = chosenSong;
    sounds.currentsong = LoadMusicStream(sounds.BackgroundSongs[chosenSong].c_str());
}

/// <summary>
/// Plays background music.
/// </summary>
void musicplayer()
{
    static bool initialized = false;

    // No music is perfectly valid: the game simply runs silently.
    if (sounds.BackgroundSongs.empty())
        return;

    if (!initialized)
    {
        musicplayerchooser();
        initialized = true;
        PlayMusicStream(sounds.currentsong);
    }

    UpdateMusicStream(sounds.currentsong);
    SetMusicVolume(sounds.currentsong, sounds.musicvolume);

    if (GetMusicTimeLength(sounds.currentsong) - GetMusicTimePlayed(sounds.currentsong) < 0.1f)
    {
        StopMusicStream(sounds.currentsong);
        UnloadMusicStream(sounds.currentsong);

        musicplayerchooser();
        PlayMusicStream(sounds.currentsong);
    }
}

int main()
{
    srand(time(0));

    Rectangle PlayArea = Rectangle{ borders.left.x + borders.left.width, borders.top.y + borders.top.height, borders.right.x -
        (borders.left.x + borders.left.width), borders.bottom.y - (borders.top.y + borders.top.height) };

    // variables (player)
    player_char.player.x = PlayArea.x + rand() % int(PlayArea.width - player_char.player.width);
    player_char.player.y = PlayArea.y + rand() % int(PlayArea.height - player_char.player.height);

    InitWindow(game_main.screenwidth, game_main.screenheight, game_main.gamename); // initializes window, a bit like python
    InitAudioDevice(); // Needed for audio
    SetTargetFPS(60); // limits FPS

    // variables (sounds)
    sounds.collectsound = LoadSound("assets/Sounds/SFX/beep.mp3");
    sounds.buttonhover = LoadSound("assets/Sounds/SFX/buttonselect.mp3");

    // variables (image) | They REQUIRE OPENGL,  It needs GPU Context, so it is absolutely NECESSARY that InitWindow is set BEFORE LoadImage
    Image mascot = LoadImage("assets/Images/ralphcutie.png"); // load image in CPU Memory (RAM)
    Texture2D Mascot = LoadTextureFromImage(mascot); // the actual image (texture)
    UnloadImage(mascot); // takes up unnecessary RAM

    orb_spawn(colors, PlayArea); // determines orb positions and colors in play area

    while (game_main.playing)
    {
        discoverbackgroundsongs();
        musicplayer();

        if (WindowShouldClose()) game_main.playing = false; // if X clicked it stops the game

        if (!game_main.inmenu)
        {
            Vector2 playerposition = { player_char.player.x, player_char.player.y };
            if (!game_main.shopping && !game_main.issettings) playermovement(playerposition, player_char.playerSpeed);

            player_char.player.x = playerposition.x;
            player_char.player.y = playerposition.y;

            // drawing
            BeginDrawing(); // this is like GUI drawing (it draws on the screen instead of world) but it WORKS for a game that is supposed to be on ONE SCREEN

            ClearBackground(BLACK); // background

            DrawRectangle(player_char.player.x, player_char.player.y, player_char.player.width, player_char.player.height, player_char.playercolor); // player

            for (int i = 0; i < orb.OrbNum; i++) // orb spawning
            {
                DrawCircle(orb.orb_rects[i].x, orb.orb_rects[i].y, orb.orb_rects[i].width / 2, orb.orbColor[i]);
            }

            // Shop icon
            DrawButton(75, 8, 100, 35, GRAY, LIGHTGRAY, 5, "SHOP", 85, 12, 30, WHITE);

            MouseHoverOver({ 75, 8, 100, 35 }, button.shophover, sounds.buttonhover, sounds.buttonhover);

            if (IsRectClick({ 75, 8, 100, 35 }, MOUSE_BUTTON_LEFT) && !game_main.issettings) game_main.shopping = not game_main.shopping;

            // calculating speed after shop
            player_char.playerSpeed = player_char.playerSpeedDef * player_char.playerSpeedMult;

            if (orb.orbrespawn) // respawns orbs
            {
                orb_spawn(colors, PlayArea);
                orb.orbrespawn = false;
            }

            // Settings icon
            DrawButton(20, 8, 35, 35, GRAY, LIGHTGRAY, 5, "S", 28, 12, 30, WHITE);

            MouseHoverOver({ 20, 8, 35, 35 }, button.settingshover, sounds.buttonhover, sounds.buttonhover);

            if (IsRectClick({ 20, 8, 35, 35 }, MOUSE_BUTTON_LEFT) && !game_main.shopping) game_main.issettings = not game_main.issettings;

            // boundaries
            DrawRectangle(borders.top.x, borders.top.y, borders.top.width, borders.top.height, RAYWHITE);
            DrawRectangle(borders.left.x, borders.left.y, borders.left.width, borders.left.height, RAYWHITE);
            DrawRectangle(borders.bottom.x, borders.bottom.y, borders.bottom.width, borders.bottom.height, RAYWHITE);
            DrawRectangle(borders.right.x, borders.right.y, borders.right.width, borders.right.height, RAYWHITE);

            // collisions
            if (player_char.player.x < borders.left.x + borders.left.width) player_char.player.x = borders.left.x + borders.left.width;
            if (player_char.player.x + player_char.player.width > borders.right.x) player_char.player.x = borders.right.x - player_char.player.width;
            if (player_char.player.y < borders.top.y + borders.top.height) player_char.player.y = borders.top.y + borders.top.height;
            if (player_char.player.y + player_char.player.height > borders.bottom.y) player_char.player.y = borders.bottom.y - player_char.player.height;

            // score system
            player_char.score += orb_collection(player_char.player, orb.orb_rects, orb.OrbNum, PlayArea, orb.orbColor, colors);
            DrawText(TextFormat("Orbs Collected: %d", player_char.score), game_main.screenwidth / 3, 0, 32, ORANGE);

            // drawn at end to be on top of everything
            if (game_main.shopping) shop(Mascot); // shop menu logic

            if (game_main.issettings) settings(Mascot); // settings menu logic

            EndDrawing(); // end drawing
        }
        else
        {
            // Drawing
            BeginDrawing();

            ClearBackground(BLACK); // background

            menu(sounds.buttonhover);

            if (game_main.issettings) settings(Mascot);

            if (game_main.iscredits) credits();

            EndDrawing();
        }
    }

    // unload textures
    UnloadTexture(Mascot);

    // unload sounds
    UnloadSound(sounds.collectsound); // this frees up memory btw
    UnloadSound(sounds.buttonhover);

    CloseAudioDevice(); // needed to stop the audio
    CloseWindow();
}