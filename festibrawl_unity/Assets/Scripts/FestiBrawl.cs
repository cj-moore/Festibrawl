// =====================================================================
// FESTIBRAWL — Holiday Mascot Brawl (Unity port)
// Single MonoBehaviour. Attach to the Main Camera. Press Play.
// =====================================================================

using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FestiBrawl : MonoBehaviour
{
    // ===== Display constants =====
    const int W = 960;
    const int H = 540;
    const float GROUND = H - 70f;
    const float GRAVITY = 0.85f;
    const float ARENA_LEFT = 40f;
    const float ARENA_RIGHT = W - 40f;

    // ===== Data classes =====
    [System.Serializable]
    public class CharDef
    {
        public string id, displayName, title, primaryHex, accentHex, skinHex;
        public string build, special, desc;
        public int hp;
        public float speed, jump, power, weight;
    }

    public class Attack
    {
        public string type;
        public int frames;
        public int hitOnStart = -1, hitOnEnd = -1;
        public float dmg, knockback, range;
        public bool hit;
    }

    public class Fighter
    {
        public CharDef chr;
        public float x, y, vx, vy;
        public int side, facing;
        public int width, height;
        public int hp, hpMax;
        public float displayHp;
        public string state = "idle";
        public int stateTimer;
        public bool onGround = true, blocking, crouching;
        public int invuln, hitstun;
        public Attack activeAttack;
        public int cooldown, poisoned;
        public bool cpu;
        public int aiTimer;
        public string aiAction = "idle";
        public int flashTimer;
        public float walkPhase;
    }

    public class Projectile
    {
        public float x, y, vx, vy;
        public string type;
        public Fighter owner;
        public string ownerId;
        public int life;
        public float dmg, w, h;
        public bool hit, gravity, poisons;
    }

    public class Particle
    {
        public float x, y, vx, vy;
        public int life;
        public Color color;
        public float size;
        public float gravity;
    }

    // ===== Roster =====
    static readonly CharDef[] ROSTER = new CharDef[]
    {
        new CharDef{ id="santa",  displayName="SAN T. CLAWS",  title="YULETIDE BRAWLER", primaryHex="#d42028", accentHex="#f5f5f5", skinHex="#ffd0a8",
                    hp=135, speed=3.4f, jump=11f,  power=1.35f, weight=1.5f,  build="huge",  special="boulder",   desc="Hurls a sack of gifts." },
        new CharDef{ id="easter", displayName="ESTHER BUNNY",  title="BASKET BANDIT",    primaryHex="#f0a0c8", accentHex="#fff080", skinHex="#ffe8d0",
                    hp=88,  speed=6.6f, jump=18f,  power=0.85f, weight=0.85f, build="lean",  special="fireball",  desc="Throws explosive eggs." },
        new CharDef{ id="jack",   displayName="JACK O'LANTERN",title="PATCH PHANTOM",    primaryHex="#ff7820", accentHex="#5a2080", skinHex="#ff7820",
                    hp=85,  speed=6f,   jump=16f,  power=1f,    weight=0.9f,  build="lean",  special="teleport",  desc="Vanishes in spirit smoke." },
        new CharDef{ id="turkey", displayName="TOM GOBBLESON", title="HARVEST KING",     primaryHex="#6a3818", accentHex="#e0a428", skinHex="#e8a060",
                    hp=110, speed=4.6f, jump=13f,  power=1.15f, weight=1.2f,  build="broad", special="tornado",   desc="Rising feather twister." },
        new CharDef{ id="cupid",  displayName="CU PID",        title="LOVE-STRUCK",      primaryHex="#ffb0c8", accentHex="#ffd040", skinHex="#ffd8c0",
                    hp=90,  speed=5.2f, jump=17f,  power=1f,    weight=0.9f,  build="small", special="lightbeam", desc="Long-range heart arrow." },
        new CharDef{ id="patty",  displayName="PATTY O'LUCKY", title="GREEN MENACE",     primaryHex="#2a8038", accentHex="#ffd040", skinHex="#f0c890",
                    hp=88,  speed=7.2f, jump=17f,  power=0.85f, weight=0.85f, build="small", special="dash",      desc="Lucky-charm rush strike." },
        new CharDef{ id="sam",    displayName="UNCLE SAM",     title="STAR-SPANGLED",    primaryHex="#2a4090", accentHex="#d42028", skinHex="#ffd8b0",
                    hp=105, speed=4.4f, jump=13f,  power=1.1f,  weight=1.15f, build="broad", special="iceshard",  desc="Fires patriot rockets." },
        new CharDef{ id="macabee",displayName="MAC A. BEE",    title="MENORAH KNIGHT",   primaryHex="#2840a0", accentHex="#e0c860", skinHex="#d8a880",
                    hp=95,  speed=5f,   jump=14f,  power=1f,    weight=1f,    build="lean",  special="poison",    desc="Spinning dreidel hex." },
    };

    static readonly string[] BACKGROUNDS = { "workshop", "graveyard", "wonderland", "fireworks" };

    // ===== Game state =====
    string state = "title";
    int frame = 0;
    float shake = 0f;
    float flash = 0f;
    int bgChoice = 0;

    // Selection
    int selP1, selP2;
    bool selP1Locked, selP2Locked;
    string selMode = "cpu";
    bool selModeChosen;

    // Fight
    Fighter[] fighters = new Fighter[2];
    List<Projectile> projectiles = new List<Projectile>();
    List<Particle> particles = new List<Particle>();
    int timerFrames = 60 * 60;
    int roundsP1, roundsP2;
    int roundEndCounter;
    string announceText = "";
    int announceTimer;

    // Rendering
    Material lineMaterial;
    GUIStyle textStyle;
    Vector2 shakeOffset = Vector2.zero;

    // Audio
    AudioSource[] audioSources;
    int audioIndex = 0;
    Dictionary<string, AudioClip> sfxClips = new Dictionary<string, AudioClip>();
    const int SFX_PLAYER_COUNT = 10;

    // ===== Lifecycle =====
    void Awake()
    {
        // GL drawing material
        var shader = Shader.Find("Hidden/Internal-Colored");
        lineMaterial = new Material(shader);
        lineMaterial.hideFlags = HideFlags.HideAndDontSave;
        lineMaterial.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
        lineMaterial.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
        lineMaterial.SetInt("_Cull", (int)UnityEngine.Rendering.CullMode.Off);
        lineMaterial.SetInt("_ZWrite", 0);

        // Audio source pool
        audioSources = new AudioSource[SFX_PLAYER_COUNT];
        for (int i = 0; i < SFX_PLAYER_COUNT; i++)
        {
            audioSources[i] = gameObject.AddComponent<AudioSource>();
            audioSources[i].playOnAwake = false;
        }

        SetupAudio();
        ResetSelection();
    }

    void Update()
    {
        frame++;
        switch (state)
        {
            case "title":     UpdateTitle(); break;
            case "select":    UpdateSelect(); break;
            case "fight":     UpdateFight(); break;
            case "roundover": UpdateRoundOver(); break;
            case "matchover": UpdateMatchOver(); break;
        }
        shake = Mathf.Max(0f, shake - 1f);
        flash = Mathf.Max(0f, flash - 1f);
        announceTimer = Mathf.Max(0, announceTimer - 1);
    }

    void OnPostRender()
    {
        if (lineMaterial == null) return;

        // Screen shake offset
        shakeOffset = Vector2.zero;
        if (shake > 0)
        {
            shakeOffset = new Vector2(
                Random.Range(-shake, shake),
                Random.Range(-shake, shake)
            ) * 0.5f;
        }

        GL.PushMatrix();
        lineMaterial.SetPass(0);
        // Pixel matrix with top-left origin (Y goes down) so our coords match the canvas
        GL.LoadPixelMatrix(0 - shakeOffset.x, W - shakeOffset.x, H - shakeOffset.y, 0 - shakeOffset.y);

        switch (state)
        {
            case "title":  DrawTitle(); break;
            case "select": DrawSelect(); break;
            default:
                DrawArena();
                DrawFightScene();
                break;
        }

        // Hit flash overlay
        if (flash > 0)
        {
            DrawRect(0, 0, W, H, new Color(1, 1, 1, flash / 4f));
        }

        // Vignette
        for (int i = 0; i < 4; i++)
        {
            float a = i * 0.12f;
            float pad = i * 12f;
            DrawRect(0, 0, W, pad, new Color(0, 0, 0, a));
            DrawRect(0, H - pad, W, pad, new Color(0, 0, 0, a));
            DrawRect(0, 0, pad, H, new Color(0, 0, 0, a));
            DrawRect(W - pad, 0, pad, H, new Color(0, 0, 0, a));
        }

        GL.PopMatrix();
    }

    void OnGUI()
    {
        // Lazy init style
        if (textStyle == null)
        {
            textStyle = new GUIStyle(GUI.skin.label);
            textStyle.alignment = TextAnchor.UpperLeft;
            textStyle.normal.textColor = Color.white;
        }

        // Scale GUI to fit 960x540 virtual space inside whatever the actual window is
        float sx = Screen.width / (float)W;
        float sy = Screen.height / (float)H;
        float s = Mathf.Min(sx, sy);
        // Center
        float ox = (Screen.width - W * s) * 0.5f;
        float oy = (Screen.height - H * s) * 0.5f;
        GUI.matrix = Matrix4x4.TRS(new Vector3(ox + shakeOffset.x * s, oy + shakeOffset.y * s, 0),
                                    Quaternion.identity, new Vector3(s, s, 1));

        switch (state)
        {
            case "title":  DrawTitleText(); break;
            case "select": DrawSelectText(); break;
            default:
                DrawFightText();
                break;
        }

        // Reset matrix
        GUI.matrix = Matrix4x4.identity;
    }

    // ===== Selection state =====
    void ResetSelection()
    {
        selP1 = 0;
        selP2 = 0;
        selP1Locked = false;
        selP2Locked = false;
        selMode = "cpu";
        selModeChosen = false;
    }

    // ===== Input helpers =====
    bool P1Pressed(string action)
    {
        switch (action)
        {
            case "left":    return Input.GetKey(KeyCode.A);
            case "right":   return Input.GetKey(KeyCode.D);
            case "up":      return Input.GetKey(KeyCode.W);
            case "down":    return Input.GetKey(KeyCode.S);
            case "punch":   return Input.GetKey(KeyCode.F);
            case "kick":    return Input.GetKey(KeyCode.G);
            case "special": return Input.GetKey(KeyCode.H);
        }
        return false;
    }
    bool P1JustPressed(string action)
    {
        switch (action)
        {
            case "left":    return Input.GetKeyDown(KeyCode.A);
            case "right":   return Input.GetKeyDown(KeyCode.D);
            case "up":      return Input.GetKeyDown(KeyCode.W);
            case "down":    return Input.GetKeyDown(KeyCode.S);
            case "punch":   return Input.GetKeyDown(KeyCode.F);
            case "kick":    return Input.GetKeyDown(KeyCode.G);
            case "special": return Input.GetKeyDown(KeyCode.H);
        }
        return false;
    }
    bool P2Pressed(string action)
    {
        switch (action)
        {
            case "left":    return Input.GetKey(KeyCode.LeftArrow);
            case "right":   return Input.GetKey(KeyCode.RightArrow);
            case "up":      return Input.GetKey(KeyCode.UpArrow);
            case "down":    return Input.GetKey(KeyCode.DownArrow);
            case "punch":   return Input.GetKey(KeyCode.J);
            case "kick":    return Input.GetKey(KeyCode.K);
            case "special": return Input.GetKey(KeyCode.L);
        }
        return false;
    }
    bool P2JustPressed(string action)
    {
        switch (action)
        {
            case "left":    return Input.GetKeyDown(KeyCode.LeftArrow);
            case "right":   return Input.GetKeyDown(KeyCode.RightArrow);
            case "up":      return Input.GetKeyDown(KeyCode.UpArrow);
            case "down":    return Input.GetKeyDown(KeyCode.DownArrow);
            case "punch":   return Input.GetKeyDown(KeyCode.J);
            case "kick":    return Input.GetKeyDown(KeyCode.K);
            case "special": return Input.GetKeyDown(KeyCode.L);
        }
        return false;
    }
    bool ConfirmJustPressed() => Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.Space) || Input.GetKeyDown(KeyCode.KeypadEnter);
    bool BackJustPressed() => Input.GetKeyDown(KeyCode.Escape);

    bool FighterPressed(int idx, string action) => idx == 0 ? P1Pressed(action) : P2Pressed(action);
    bool FighterJustPressed(int idx, string action) => idx == 0 ? P1JustPressed(action) : P2JustPressed(action);

    // ===== TITLE =====
    void UpdateTitle()
    {
        if (ConfirmJustPressed())
        {
            PlaySfx("confirm");
            state = "select";
            ResetSelection();
        }
    }

    void DrawTitle()
    {
        DrawRect(0, 0, W, H, H1("#0a0510"));
        // Confetti
        for (int i = 0; i < 50; i++)
        {
            float x = Mod(i * 73f + frame * 0.5f, W);
            float y = Mod(i * 37f, H);
            Color c;
            switch (i % 4)
            {
                case 0: c = H1("#ff4040"); break;
                case 1: c = H1("#40dd40"); break;
                case 2: c = H1("#ffaa40"); break;
                default: c = H1("#80c0ff"); break;
            }
            c.a = 0.25f;
            DrawRect(x, y, 3, 3, c);
        }

        // Demo fighters
        int idxL = (frame / 120) % ROSTER.Length;
        int idxR = (idxL + 4) % ROSTER.Length;
        float bob = Mathf.Sin(frame * 0.05f) * 4f;
        DrawFighterSprite(W / 2f - 240, 380 + bob, ROSTER[idxL], 1, "idle", 0, 1f, null, 0f, false, false);
        DrawFighterSprite(W / 2f + 240, 380 - bob, ROSTER[idxR], -1, "idle", 0, 1f, null, 0f, false, false);
    }

    void DrawTitleText()
    {
        // Title text via GUI
        SetText(56, H1("#ffd040"), TextAnchor.MiddleCenter);
        // Shadow stack approximation via repeated draws
        for (int i = 6; i > 0; i--)
        {
            SetText(56, new Color(0.3f + i * 0.04f, 0.05f + i * 0.02f, 0.05f + i * 0.02f), TextAnchor.MiddleCenter);
            DrawTextCentered("FESTIBRAWL", H / 2f - 80 + i, i);
        }
        SetText(56, H1("#ffd040"), TextAnchor.MiddleCenter);
        DrawTextCentered("FESTIBRAWL", H / 2f - 80, 0);

        SetText(14, H1("#ff6644"), TextAnchor.MiddleCenter);
        DrawTextCentered("— HOLIDAY MASCOT BRAWL —", 220, 0);

        SetText(48, H1("#ff4040"), TextAnchor.MiddleCenter);
        DrawTextCentered("VS", 380, 0);

        if ((frame / 30) % 2 == 0)
        {
            SetText(16, H1("#f5e9c4"), TextAnchor.MiddleCenter);
            DrawTextCentered("PRESS ENTER TO START", 480, 0);
        }
        SetText(9, H1("#8a7a5a"), TextAnchor.MiddleCenter);
        DrawTextCentered("(C) 2026  FESTIBRAWL  |  8 MASCOTS  |  BEST OF 3", 515, 0);
    }

    // ===== SELECT =====
    void UpdateSelect()
    {
        if (!selModeChosen)
        {
            if (P1JustPressed("left") || P2JustPressed("left")) { selMode = "cpu"; PlaySfx("select"); }
            if (P1JustPressed("right") || P2JustPressed("right")) { selMode = "2p"; PlaySfx("select"); }
            if (ConfirmJustPressed()) { selModeChosen = true; PlaySfx("confirm"); }
            if (BackJustPressed()) { state = "title"; PlaySfx("select"); }
            return;
        }

        if (!selP1Locked)
        {
            if (P1JustPressed("left"))  { selP1 = (selP1 + ROSTER.Length - 1) % ROSTER.Length; PlaySfx("select"); }
            if (P1JustPressed("right")) { selP1 = (selP1 + 1) % ROSTER.Length; PlaySfx("select"); }
            if (P1JustPressed("up"))    { selP1 = (selP1 + ROSTER.Length - 4) % ROSTER.Length; PlaySfx("select"); }
            if (P1JustPressed("down"))  { selP1 = (selP1 + 4) % ROSTER.Length; PlaySfx("select"); }
            if (P1JustPressed("punch") || ConfirmJustPressed()) { selP1Locked = true; PlaySfx("confirm"); }
        }

        if (selP1Locked && !selP2Locked)
        {
            if (selMode == "2p")
            {
                if (P2JustPressed("left"))  { selP2 = (selP2 + ROSTER.Length - 1) % ROSTER.Length; PlaySfx("select"); }
                if (P2JustPressed("right")) { selP2 = (selP2 + 1) % ROSTER.Length; PlaySfx("select"); }
                if (P2JustPressed("up"))    { selP2 = (selP2 + ROSTER.Length - 4) % ROSTER.Length; PlaySfx("select"); }
                if (P2JustPressed("down"))  { selP2 = (selP2 + 4) % ROSTER.Length; PlaySfx("select"); }
                if (P2JustPressed("punch") || ConfirmJustPressed()) { selP2Locked = true; PlaySfx("confirm"); }
            }
            else
            {
                selP2 = (selP1 + 1 + Random.Range(0, ROSTER.Length - 1)) % ROSTER.Length;
                selP2Locked = true;
                PlaySfx("confirm");
            }
        }

        if (selP1Locked && selP2Locked)
        {
            bgChoice = Random.Range(0, BACKGROUNDS.Length);
            state = "fight";
            roundsP1 = 0;
            roundsP2 = 0;
            StartRound();
        }

        if (BackJustPressed())
        {
            if (selP2Locked) selP2Locked = false;
            else if (selP1Locked) selP1Locked = false;
            else if (selModeChosen) selModeChosen = false;
            else state = "title";
            PlaySfx("select");
        }
    }

    void DrawSelect()
    {
        DrawRect(0, 0, W, H, H1("#0a0510"));
        Color grid = H1("#2a1a3a");
        for (int i = 0; i < W; i += 30) DrawLine(i, 0, i, H, grid, 1);
        for (int j = 0; j < H; j += 30) DrawLine(0, j, W, j, grid, 1);

        if (!selModeChosen)
        {
            // Mode selection cards
            for (int i = 0; i < 2; i++)
            {
                string val = i == 0 ? "cpu" : "2p";
                float x = W / 2f + (i == 0 ? -160f : 160f);
                bool selected = selMode == val;
                Color bg = selected ? H1("#ff4040") : H1("#3a2a4a");
                DrawRect(x - 130, 240, 260, 90, bg);
                Color border = selected ? H1("#ffd040") : H1("#6a4a7a");
                StrokeRect(x - 130, 240, 260, 90, border, 3);
            }
            return;
        }

        // Character grid
        int cols = 4;
        float cellW = 130, cellH = 130;
        float gridX = (W - cols * cellW) / 2f;
        float gridY = 80f;
        for (int i = 0; i < ROSTER.Length; i++)
        {
            CharDef c = ROSTER[i];
            float cx = gridX + (i % cols) * cellW;
            float cy = gridY + (i / cols) * cellH;
            DrawRect(cx + 5, cy + 5, cellW - 10, cellH - 10, H1("#1a0a25"));

            bool isP1 = (i == selP1);
            bool isP2 = (i == selP2);
            if (isP1 && !selP1Locked)
            {
                Color col = (frame % 20 < 10) ? H1("#ff4040") : H1("#ffaa40");
                StrokeRect(cx + 5, cy + 5, cellW - 10, cellH - 10, col, 4);
            }
            else if (isP1 && selP1Locked)
            {
                StrokeRect(cx + 5, cy + 5, cellW - 10, cellH - 10, H1("#ff4040"), 4);
            }
            if (selP1Locked && selMode == "2p" && isP2)
            {
                Color col2 = !selP2Locked ? ((frame % 20 < 10) ? H1("#4080ff") : H1("#80c0ff")) : H1("#4080ff");
                StrokeRect(cx + 8, cy + 8, cellW - 16, cellH - 16, col2, 4);
            }

            Color swatch = H1(c.primaryHex);
            swatch.a = 0.18f;
            DrawRect(cx + 12, cy + 12, cellW - 24, cellH - 50, swatch);
            DrawFighterSprite(cx + cellW / 2f, cy + 90, c, 1, "idle", 0, 0.55f, null, 0f, false, false);
        }

        DrawSidePanel(40, 240, ROSTER[selP1], H1("#ff4040"), selP1Locked);
        bool p2Ready = selP2Locked || (selMode == "cpu" && selP1Locked);
        DrawSidePanel(W - 220, 240, ROSTER[selP2], H1("#4080ff"), p2Ready);
    }

    void DrawSelectText()
    {
        if (!selModeChosen)
        {
            SetText(24, H1("#ffd040"), TextAnchor.MiddleCenter);
            DrawTextCentered("SELECT MODE", 130, 0);
            SetText(20, Color.white, TextAnchor.MiddleCenter);
            DrawTextAt("VS CPU", W / 2f - 160, 280, 260);
            DrawTextAt("VS PLAYER 2", W / 2f + 160, 280, 260);
            SetText(12, H1("#8a7a5a"), TextAnchor.MiddleCenter);
            DrawTextCentered("LEFT/RIGHT  ENTER to confirm", 420, 0);
            return;
        }

        SetText(20, H1("#ffd040"), TextAnchor.MiddleCenter);
        DrawTextCentered("SELECT YOUR FIGHTER", 50, 0);

        // Character names
        int cols = 4;
        float cellW = 130, cellH = 130;
        float gridX = (W - cols * cellW) / 2f;
        float gridY = 80f;
        SetText(10, Color.white, TextAnchor.MiddleCenter);
        for (int i = 0; i < ROSTER.Length; i++)
        {
            float cx = gridX + (i % cols) * cellW;
            float cy = gridY + (i / cols) * cellH;
            DrawTextAt(ROSTER[i].displayName, cx + cellW / 2f, cy + cellH - 12, cellW);
        }

        DrawSidePanelText(40, 240, ROSTER[selP1], "P1", selP1Locked);
        bool p2Ready = selP2Locked || (selMode == "cpu" && selP1Locked);
        DrawSidePanelText(W - 220, 240, ROSTER[selP2], "P2", p2Ready);

        SetText(10, H1("#8a7a5a"), TextAnchor.MiddleCenter);
        if (!selP1Locked) DrawTextCentered("P1: WASD navigate, F to lock in", H - 30, 0);
        else if (!selP2Locked && selMode == "2p") DrawTextCentered("P2: ARROWS navigate, J to lock in", H - 30, 0);
        else DrawTextCentered("LOADING...", H - 30, 0);
    }

    void DrawSidePanel(float x, float y, CharDef c, Color color, bool locked)
    {
        DrawRect(x, y, 180, 180, H1("#1a0a25"));
        StrokeRect(x, y, 180, 180, color, 3);
        DrawRect(x, y, 180, 24, color);

        // Stat bars
        DrawStatBar(x + 10, y + 90, c.hp / 140f);
        DrawStatBar(x + 10, y + 108, c.speed / 8f);
        DrawStatBar(x + 10, y + 126, c.power / 1.5f);
    }

    void DrawSidePanelText(float x, float y, CharDef c, string label, bool locked)
    {
        SetText(12, Color.white, TextAnchor.MiddleLeft);
        DrawTextAtRaw(label + (locked ? " READY" : ""), x + 10, y + 6, 160);
        SetText(12, H1(c.primaryHex), TextAnchor.MiddleLeft);
        DrawTextAtRaw(c.displayName, x + 10, y + 40, 160);
        SetText(8, H1("#aaaaaa"), TextAnchor.MiddleLeft);
        DrawTextAtRaw(c.title, x + 10, y + 60, 160);

        SetText(7, H1("#dddddd"), TextAnchor.MiddleLeft);
        DrawTextAtRaw("HP",  x + 10, y + 86, 30);
        DrawTextAtRaw("SPD", x + 10, y + 104, 30);
        DrawTextAtRaw("PWR", x + 10, y + 122, 30);

        // Description (simple wrap by char count)
        var words = c.desc.Split(' ');
        var lines = new List<string>();
        var line = "";
        foreach (var w in words)
        {
            if ((line + w).Length > 24) { lines.Add(line); line = w + " "; }
            else line += w + " ";
        }
        lines.Add(line);
        for (int i = 0; i < lines.Count; i++)
            DrawTextAtRaw(lines[i].Trim(), x + 10, y + 144 + i * 10, 160);
    }

    void DrawStatBar(float x, float y, float frac)
    {
        float bx = x + 32, bw = 120;
        DrawRect(bx, y, bw, 10, Color.black);
        DrawRect(bx, y, bw * Mathf.Clamp01(frac), 10, H1("#ffd040"));
        StrokeRect(bx, y, bw, 10, H1("#6a4a1a"), 1);
    }

    // ===== FIGHT =====
    void StartRound()
    {
        fighters[0] = MakeFighter(ROSTER[selP1], 220, 1, false);
        fighters[1] = MakeFighter(ROSTER[selP2], W - 220, -1, selMode == "cpu");
        fighters[0].facing = 1;
        fighters[1].facing = -1;
        projectiles.Clear();
        particles.Clear();
        timerFrames = 60 * 60;
        roundEndCounter = 0;
        Announce("ROUND " + (roundsP1 + roundsP2 + 1), 90);
        StartCoroutine(AnnounceFight());
    }

    IEnumerator AnnounceFight()
    {
        yield return new WaitForSeconds(1.5f);
        Announce("FIGHT!", 60);
    }

    Fighter MakeFighter(CharDef c, float x, int side, bool cpu)
    {
        int width, height;
        switch (c.build)
        {
            case "huge":  width = 56; height = 110; break;
            case "broad": width = 50; height = 100; break;
            case "small": width = 38; height = 86; break;
            default:      width = 44; height = 100; break;
        }
        return new Fighter
        {
            chr = c,
            x = x, y = GROUND,
            side = side, facing = side,
            width = width, height = height,
            hp = c.hp, hpMax = c.hp,
            displayHp = c.hp,
            cpu = cpu,
        };
    }

    void Announce(string text, int frames)
    {
        announceText = text;
        announceTimer = frames;
    }

    void UpdateFight()
    {
        if (announceTimer > 30) return;
        timerFrames = Mathf.Max(0, timerFrames - 1);

        if (fighters[0].onGround) fighters[0].facing = fighters[0].x < fighters[1].x ? 1 : -1;
        if (fighters[1].onGround) fighters[1].facing = fighters[1].x < fighters[0].x ? 1 : -1;

        for (int i = 0; i < 2; i++)
        {
            var f = fighters[i];
            if (f.state == "ko") continue;
            if (f.cpu) AiInput(f, fighters[1 - i]);
            else HandleInput(f, i);
        }

        foreach (var f in fighters) UpdateFighter(f);

        // Projectiles
        for (int i = projectiles.Count - 1; i >= 0; i--)
        {
            var p = projectiles[i];
            p.x += p.vx;
            p.y += p.vy;
            if (p.gravity) p.vy += 0.4f;
            p.life--;
            foreach (var f in fighters)
            {
                if (f == p.owner || f.state == "ko" || f.invuln > 0 || p.hit) continue;
                float dx = Mathf.Abs(f.x - p.x);
                float dy = Mathf.Abs((f.y - 50f) - p.y);
                if (dx < (f.width / 2f + p.w / 2f) && dy < (f.height / 2f + p.h / 2f))
                {
                    ApplyHit(f, p.dmg, Mathf.Sign(p.vx) * 5f, p.owner.chr.power);
                    if (p.poisons) f.poisoned = 180;
                    p.hit = true;
                    p.life = 0;
                    Color color;
                    switch (p.type)
                    {
                        case "fire": color = H1("#ffaa20"); break;
                        case "ice":  color = H1("#a8e0ff"); break;
                        case "rock": color = H1("#8a6a3a"); break;
                        case "beam": color = H1("#ffe080"); break;
                        default:     color = H1("#80ff40"); break;
                    }
                    SpawnHitParticles(p.x, p.y, color, 8);
                    break;
                }
            }
            if (p.life <= 0 || p.x < 0 || p.x > W || p.y > GROUND + 20)
                projectiles.RemoveAt(i);
        }

        // Melee hits
        foreach (var f in fighters)
        {
            if (f.activeAttack == null || f.activeAttack.hit) continue;
            var a = f.activeAttack;
            if (a.hitOnStart >= 0 && a.hitOnEnd >= 0)
            {
                int elapsed = a.frames - f.stateTimer;
                if (elapsed >= a.hitOnStart && elapsed <= a.hitOnEnd)
                {
                    var opp = Other(f);
                    if (opp.state != "ko" && opp.invuln <= 0)
                    {
                        float dx = Mathf.Abs(f.x - opp.x);
                        if (dx < a.range && Mathf.Abs(f.y - opp.y) < 90 && Mathf.Sign(opp.x - f.x) == f.facing)
                        {
                            ApplyHit(opp, a.dmg, f.facing * a.knockback, f.chr.power);
                            a.hit = true;
                        }
                    }
                }
            }
        }

        // Particles
        for (int i = particles.Count - 1; i >= 0; i--)
        {
            var p = particles[i];
            p.x += p.vx;
            p.y += p.vy;
            p.vy += p.gravity;
            p.life--;
            if (p.life <= 0) particles.RemoveAt(i);
        }

        // Poison DoT
        foreach (var f in fighters)
        {
            if (f.poisoned > 0 && f.state != "ko")
            {
                f.poisoned--;
                if (f.poisoned % 30 == 0)
                {
                    f.hp = Mathf.Max(0, f.hp - 2);
                    SpawnHitParticles(f.x, f.y - 50, H1("#80ff40"), 4);
                    if (f.hp <= 0)
                    {
                        f.state = "ko";
                        f.stateTimer = 999;
                        PlaySfx("ko");
                    }
                }
            }
        }

        // Round-end check
        if (fighters[0].state == "ko" || fighters[1].state == "ko" || timerFrames == 0)
        {
            state = "roundover";
            roundEndCounter = 150;
            int winner = -1;
            if (fighters[0].state == "ko" && fighters[1].state == "ko") winner = -1;
            else if (fighters[0].state == "ko") { winner = 1; roundsP2++; }
            else if (fighters[1].state == "ko") { winner = 0; roundsP1++; }
            else if (fighters[0].hp > fighters[1].hp) { winner = 0; roundsP1++; }
            else if (fighters[1].hp > fighters[0].hp) { winner = 1; roundsP2++; }

            if (winner == 0) Announce(fighters[0].chr.displayName + " WINS!", roundEndCounter);
            else if (winner == 1) Announce(fighters[1].chr.displayName + " WINS!", roundEndCounter);
            else Announce("DRAW!", roundEndCounter);
        }
    }

    Fighter Other(Fighter f) => f == fighters[0] ? fighters[1] : fighters[0];

    void HandleInput(Fighter f, int idx)
    {
        if (f.state == "hit" || f.state == "punch" || f.state == "kick" || f.state == "special")
        {
            f.blocking = false;
            return;
        }
        f.blocking = FighterPressed(idx, "down") && f.onGround;

        if (f.onGround && !f.blocking)
        {
            bool moving = false;
            if (FighterPressed(idx, "left") && f.x > ARENA_LEFT)  { f.vx = -f.chr.speed; moving = true; }
            else if (FighterPressed(idx, "right") && f.x < ARENA_RIGHT) { f.vx = f.chr.speed; moving = true; }
            else { f.vx *= 0.6f; }
            f.crouching = FighterPressed(idx, "down") && !FighterPressed(idx, "left") && !FighterPressed(idx, "right");
            if (FighterJustPressed(idx, "up"))
            {
                f.vy = -f.chr.jump;
                f.onGround = false;
                PlaySfx("jump");
            }
            if (moving && f.state != "walk" && !f.crouching) f.state = "walk";
            else if (!moving && !f.crouching && f.state == "walk") f.state = "idle";
            if (f.crouching) f.state = "crouch";
            else if (!moving && f.state == "crouch") f.state = "idle";
        }
        else if (f.blocking)
        {
            f.vx *= 0.5f;
            f.state = "block";
        }
        else
        {
            // Air control
            if (FighterPressed(idx, "left"))  f.vx = Mathf.Max(f.vx - 0.4f, -f.chr.speed * 0.7f);
            if (FighterPressed(idx, "right")) f.vx = Mathf.Min(f.vx + 0.4f,  f.chr.speed * 0.7f);
        }

        if (FighterJustPressed(idx, "punch"))   StartAttack(f, "punch");
        if (FighterJustPressed(idx, "kick"))    StartAttack(f, "kick");
        if (FighterJustPressed(idx, "special")) StartAttack(f, "special");
    }

    void AiInput(Fighter f, Fighter opp)
    {
        f.aiTimer--;
        float dx = opp.x - f.x;
        float dist = Mathf.Abs(dx);
        int dir = dx > 0 ? 1 : -1;

        if (f.aiTimer <= 0)
        {
            f.aiTimer = 20 + Random.Range(0, 40);
            float r = Random.value;
            if (dist > 220)
            {
                if (r < 0.35f && f.cooldown <= 0) f.aiAction = "special";
                else f.aiAction = "approach";
            }
            else if (dist > 90)
            {
                if (r < 0.35f) f.aiAction = "jump_in";
                else if (r < 0.7f) f.aiAction = "approach";
                else f.aiAction = "wait";
            }
            else
            {
                if (r < 0.45f) f.aiAction = "attack";
                else if (r < 0.6f) f.aiAction = "block";
                else if (r < 0.75f) f.aiAction = "retreat";
                else f.aiAction = "jump_back";
            }
        }

        f.blocking = false;
        if (f.state == "hit" || f.state == "punch" || f.state == "kick" || f.state == "special") return;

        switch (f.aiAction)
        {
            case "approach":
                if (f.onGround && f.x > ARENA_LEFT && f.x < ARENA_RIGHT)
                    f.vx = dir * f.chr.speed * 0.85f;
                f.state = "walk";
                break;
            case "retreat":
                if (f.onGround && f.x > ARENA_LEFT + 10 && f.x < ARENA_RIGHT - 10)
                    f.vx = -dir * f.chr.speed * 0.7f;
                f.state = "walk";
                break;
            case "jump_in":
                if (f.onGround) { f.vy = -f.chr.jump; f.vx = dir * f.chr.speed * 0.6f; f.onGround = false; PlaySfx("jump"); }
                break;
            case "jump_back":
                if (f.onGround) { f.vy = -f.chr.jump; f.vx = -dir * f.chr.speed * 0.6f; f.onGround = false; PlaySfx("jump"); }
                break;
            case "attack":
                StartAttack(f, Random.value < 0.5f ? "punch" : "kick");
                break;
            case "special":
                StartAttack(f, "special");
                break;
            case "block":
                f.blocking = true;
                f.vx *= 0.5f;
                f.state = "block";
                break;
            case "wait":
                f.vx *= 0.6f;
                f.state = "idle";
                break;
        }
    }

    void StartAttack(Fighter f, string type)
    {
        if (f.state == "hit" || f.state == "ko" || f.activeAttack != null) return;
        if (f.cooldown > 0 && type == "special") return;
        if (!f.onGround && type != "punch" && type != "kick") return;

        Attack atk = null;
        if (type == "punch")
        {
            atk = new Attack { type = type, frames = 18, hitOnStart = 6, hitOnEnd = 9, dmg = 6, knockback = 3, range = 56 };
            PlaySfx("whoosh");
        }
        else if (type == "kick")
        {
            atk = new Attack { type = type, frames = 26, hitOnStart = 10, hitOnEnd = 16, dmg = 11, knockback = 6, range = 70 };
            PlaySfx("whoosh");
        }
        else
        {
            atk = StartSpecial(f);
            if (atk == null) return;
            f.cooldown = 90;
        }
        f.activeAttack = atk;
        f.state = type;
        f.stateTimer = atk.frames;
    }

    Attack StartSpecial(Fighter f)
    {
        var c = f.chr;
        int dir = f.facing;
        switch (c.special)
        {
            case "fireball":
                PlaySfx("fire");
                StartCoroutine(DelayedSpawn(f, "fire", 0.28f));
                return new Attack { type = "special", frames = 36 };
            case "iceshard":
                PlaySfx("fire");
                StartCoroutine(DelayedSpawn(f, "ice", 0.35f));
                return new Attack { type = "special", frames = 44 };
            case "boulder":
                PlaySfx("heavy");
                StartCoroutine(DelayedSpawn(f, "rock", 0.40f));
                return new Attack { type = "special", frames = 50 };
            case "lightbeam":
                PlaySfx("fire");
                StartCoroutine(DelayedSpawn(f, "beam", 0.30f));
                return new Attack { type = "special", frames = 40 };
            case "poison":
                PlaySfx("fire");
                StartCoroutine(DelayedSpawn(f, "poison", 0.28f));
                return new Attack { type = "special", frames = 36 };
            case "dash":
                PlaySfx("whoosh");
                f.invuln = 18;
                f.vx = dir * 18f;
                return new Attack { type = "special", frames = 22, hitOnStart = 2, hitOnEnd = 18, dmg = 14, knockback = 8, range = 60 };
            case "tornado":
                PlaySfx("whoosh");
                f.vy = -16f;
                f.onGround = false;
                f.vx = dir * 4f;
                return new Attack { type = "special", frames = 30, hitOnStart = 2, hitOnEnd = 28, dmg = 10, knockback = 6, range = 70 };
            case "teleport":
                PlaySfx("whoosh");
                var opp = Other(f);
                f.x = opp.x - opp.facing * 70f;
                f.facing = -opp.facing;
                f.invuln = 20;
                for (int i = 0; i < 12; i++) particles.Add(MakeParticle(f.x, f.y - 50, H1("#a040ff")));
                return new Attack { type = "special", frames = 28, hitOnStart = 18, hitOnEnd = 24, dmg = 12, knockback = 6, range = 60 };
        }
        return null;
    }

    IEnumerator DelayedSpawn(Fighter f, string type, float delay)
    {
        yield return new WaitForSeconds(delay);
        if (f != null && f.state != "ko") SpawnProjectile(f, type);
    }

    void SpawnProjectile(Fighter f, string type)
    {
        int dir = f.facing;
        float px = f.x + dir * 40f;
        float py = f.y - 60f;
        var p = new Projectile
        {
            x = px, y = py, vx = dir * 8f, vy = 0,
            type = type, owner = f, ownerId = f.chr.id,
            life = 100, dmg = 12, w = 24, h = 18,
        };
        switch (type)
        {
            case "rock":
                p.vx = dir * 6f; p.vy = -4f; p.dmg = 16; p.w = 30; p.h = 28;
                p.gravity = true; p.life = 90;
                break;
            case "beam":
                p.vx = dir * 14f; p.dmg = 10; p.w = 50; p.h = 12; p.life = 60;
                break;
            case "poison":
                p.vx = dir * 5f; p.dmg = 8; p.life = 140; p.poisons = true;
                break;
            case "ice":
                p.dmg = 14; p.w = 26; p.h = 14;
                break;
        }
        projectiles.Add(p);
    }

    Particle MakeParticle(float x, float y, Color color)
    {
        return new Particle
        {
            x = x, y = y,
            vx = (Random.value - 0.5f) * 6f,
            vy = -Random.value * 5f - 1f,
            life = 30 + Random.Range(0, 20),
            color = color,
            size = 2f + Random.value * 3f,
            gravity = 0.2f,
        };
    }

    void SpawnHitParticles(float x, float y, Color color, int n)
    {
        for (int i = 0; i < n; i++) particles.Add(MakeParticle(x, y, color));
    }

    void UpdateFighter(Fighter f)
    {
        if (f.state == "ko")
        {
            f.vy += GRAVITY;
            f.y += f.vy;
            f.x += f.vx * 0.5f;
            if (f.y >= GROUND) { f.y = GROUND; f.vy = 0; f.vx *= 0.7f; }
            return;
        }
        f.x += f.vx;
        f.y += f.vy;
        if (!f.onGround) f.vy += GRAVITY;
        f.x = Mathf.Clamp(f.x, ARENA_LEFT, ARENA_RIGHT);
        if (f.y >= GROUND)
        {
            f.y = GROUND;
            f.vy = 0;
            if (!f.onGround)
            {
                f.onGround = true;
                if (f.state == "jump" || f.state == "fall") f.state = "idle";
            }
        }
        else
        {
            f.onGround = false;
            if (f.state != "punch" && f.state != "kick" && f.state != "special" && f.state != "hit")
                f.state = f.vy < 0 ? "jump" : "fall";
        }
        if (f.state == "walk") f.walkPhase += 0.25f;
        if (f.cooldown > 0) f.cooldown--;
        if (f.invuln > 0) f.invuln--;
        if (f.flashTimer > 0) f.flashTimer--;
        if (f.hitstun > 0) f.hitstun--;
        if (f.activeAttack != null)
        {
            f.stateTimer--;
            if (f.stateTimer <= 0)
            {
                f.activeAttack = null;
                f.state = f.onGround ? "idle" : "fall";
            }
        }
        else if (f.state == "hit")
        {
            f.stateTimer--;
            if (f.stateTimer <= 0) f.state = f.onGround ? "idle" : "fall";
        }

        // Push apart if overlapping
        var opp = Other(f);
        if (f.onGround && opp.onGround)
        {
            float dx = f.x - opp.x;
            float minDist = (f.width + opp.width) / 2f - 4f;
            if (Mathf.Abs(dx) < minDist)
            {
                float push = (minDist - Mathf.Abs(dx)) / 2f;
                f.x += Mathf.Sign(dx) * push * 0.5f;
            }
        }
    }

    void ApplyHit(Fighter f, float dmg, float kb, float powerMul)
    {
        if (f.invuln > 0 || f.state == "ko") return;
        if (f.blocking && Mathf.Sign(kb) != f.facing)
        {
            f.hp = Mathf.Max(0, f.hp - Mathf.RoundToInt(dmg * 0.15f));
            f.vx += kb * 0.3f;
            PlaySfx("block");
            SpawnHitParticles(f.x, f.y - 60, Color.white, 4);
            return;
        }
        int finalDmg = Mathf.RoundToInt(dmg * powerMul);
        f.hp = Mathf.Max(0, f.hp - finalDmg);
        f.vx += kb;
        f.vy = -4;
        f.onGround = false;
        f.state = "hit";
        f.stateTimer = 18;
        f.flashTimer = 8;
        f.invuln = 14;
        f.activeAttack = null;
        Color pcol = finalDmg > 10 ? H1("#ff4040") : H1("#ffaa40");
        SpawnHitParticles(f.x, f.y - 60, pcol, finalDmg > 10 ? 12 : 6);
        shake = Mathf.Min(20f, shake + (finalDmg > 10 ? 12f : 5f));
        flash = finalDmg > 10 ? 4 : 2;
        PlaySfx(finalDmg > 10 ? "heavy" : "hit");
        if (f.hp <= 0)
        {
            f.state = "ko";
            f.stateTimer = 999;
            f.vx = -f.facing * 8f;
            f.vy = -10f;
            f.onGround = false;
            PlaySfx("ko");
            shake = 30;
        }
    }

    void UpdateRoundOver()
    {
        foreach (var f in fighters) UpdateFighter(f);
        for (int i = particles.Count - 1; i >= 0; i--)
        {
            var p = particles[i];
            p.x += p.vx; p.y += p.vy; p.vy += 0.2f; p.life--;
            if (p.life <= 0) particles.RemoveAt(i);
        }
        roundEndCounter--;
        if (roundEndCounter <= 0)
        {
            if (roundsP1 >= 2 || roundsP2 >= 2)
            {
                state = "matchover";
                Announce(roundsP1 > roundsP2 ? "PLAYER 1 VICTORY" : "PLAYER 2 VICTORY", 9999);
            }
            else
            {
                state = "fight";
                StartRound();
            }
        }
    }

    void UpdateMatchOver()
    {
        if (ConfirmJustPressed())
        {
            PlaySfx("confirm");
            state = "select";
            string prevMode = selMode;
            ResetSelection();
            selMode = prevMode;
            selModeChosen = true;
        }
        if (BackJustPressed()) { PlaySfx("select"); state = "title"; }
    }

    // ===== DRAW: FIGHT SCENE =====
    void DrawFightScene()
    {
        // Particles behind
        foreach (var p in particles)
        {
            Color c = p.color;
            c.a = Mathf.Clamp01(p.life / 30f);
            DrawRect(p.x - p.size / 2f, p.y - p.size / 2f, p.size, p.size, c);
        }
        // Shadows
        foreach (var f in fighters)
        {
            float sw = f.width * (f.onGround ? 1f : 0.6f);
            DrawEllipse(f.x, GROUND + 5, sw / 2f, 6f, new Color(0, 0, 0, 0.4f));
        }
        // Fighters
        foreach (var f in fighters) DrawFighter(f);
        // Projectiles
        foreach (var p in projectiles) DrawProjectile(p);
        // HUD bars (no text yet — text is in OnGUI)
        DrawHUDBars();

        // Match over overlay
        if (state == "matchover")
            DrawRect(0, 0, W, H, new Color(0, 0, 0, 0.7f));
    }

    void DrawFightText()
    {
        // Names
        SetText(14, Color.white, TextAnchor.MiddleLeft);
        DrawTextAtRaw(fighters[0].chr.displayName, 20, 70, 280);
        SetText(14, Color.white, TextAnchor.MiddleRight);
        DrawTextAtRaw(fighters[1].chr.displayName, W - 300, 70, 280);

        // Timer
        int seconds = Mathf.CeilToInt(timerFrames / 60f);
        Color tcol;
        if (seconds <= 10) tcol = (frame % 20 < 10) ? H1("#ff4040") : H1("#ffaa40");
        else tcol = H1("#ffd040");
        SetText(28, tcol, TextAnchor.MiddleCenter);
        DrawTextCentered(seconds.ToString("00"), 38, 0);

        // HP labels
        SetText(10, Color.white, TextAnchor.MiddleLeft);
        DrawTextAtRaw("HP " + fighters[0].hp + "/" + fighters[0].hpMax, 26, 30, 200);
        SetText(10, Color.white, TextAnchor.MiddleRight);
        DrawTextAtRaw("HP " + fighters[1].hp + "/" + fighters[1].hpMax, W - 326, 30, 200);

        // Special label
        SetText(8, fighters[0].cooldown > 0 ? H1("#666666") : H1("#ffd040"), TextAnchor.MiddleLeft);
        DrawTextAtRaw("SPECIAL", 20, 92, 60);
        SetText(8, fighters[1].cooldown > 0 ? H1("#666666") : H1("#ffd040"), TextAnchor.MiddleLeft);
        DrawTextAtRaw("SPECIAL", W - 60, 92, 60);

        // Announcement
        if (announceTimer > 0)
        {
            float alpha = Mathf.Clamp01(announceTimer / 30f);
            Color tc = new Color(1f, 0.82f, 0.25f, alpha);
            // Shadow
            for (int i = 5; i > 0; i--)
            {
                SetText(56, new Color(0, 0, 0, 0.5f * alpha), TextAnchor.MiddleCenter);
                DrawTextCentered(announceText, H / 2f - 20 + i, i);
            }
            SetText(56, tc, TextAnchor.MiddleCenter);
            DrawTextCentered(announceText, H / 2f - 20, 0);
        }

        if (state == "matchover")
        {
            SetText(36, H1("#ffd040"), TextAnchor.MiddleCenter);
            DrawTextCentered(announceText, H / 2f - 20, 0);
            if ((frame / 30) % 2 == 0)
            {
                SetText(14, Color.white, TextAnchor.MiddleCenter);
                DrawTextCentered("PRESS ENTER FOR REMATCH  |  ESC TO TITLE", H / 2f + 40, 0);
            }
        }
    }

    void DrawFighter(Fighter f)
    {
        bool flashing = f.flashTimer > 0 && (frame % 4 < 2);
        DrawFighterSprite(f.x, f.y, f.chr, f.facing, f.state, f.stateTimer, 1f,
            f.activeAttack, f.walkPhase, flashing, f.poisoned > 0);
    }

    void DrawProjectile(Projectile p)
    {
        int dir = p.vx > 0 ? 1 : (p.vx < 0 ? -1 : 1);
        string oid = p.ownerId;

        if (oid == "easter" && p.type == "fire")
        {
            float wob = Mathf.Sin(frame * 0.3f) * 1.5f;
            DrawEllipse(p.x, p.y + wob, 12, 16, H1("#fff080"));
            DrawRect(p.x - 11, p.y - 5 + wob, 22, 3, H1("#80c0e8"));
            DrawRect(p.x - 10, p.y + 4 + wob, 20, 2, H1("#80c0e8"));
            DrawRect(p.x - 8, p.y - 10 + wob, 16, 2, H1("#ff80a0"));
            DrawRect(p.x - 4, p.y - 12 + wob, 3, 2, Color.white);
            Color sparkle = H1("#ffe0a0"); sparkle.a = 0.6f;
            for (int i = 0; i < 3; i++)
                DrawRect(p.x - dir * (8 + i * 5), p.y + Mathf.Sin(frame * 0.4f + i) * 3, 2, 2, sparkle);
        }
        else if (oid == "santa" && p.type == "rock")
        {
            DrawRect(p.x - 16, p.y - 14, 32, 28, H1("#a01820"));
            DrawRect(p.x - 14, p.y - 12, 28, 24, H1("#cc2030"));
            DrawRect(p.x - 16, p.y - 3, 32, 5, H1("#ffd040"));
            DrawRect(p.x - 3, p.y - 14, 5, 28, H1("#ffd040"));
            DrawCircle(p.x - 5, p.y - 16, 4, H1("#ffd040"));
            DrawCircle(p.x + 5, p.y - 16, 4, H1("#ffd040"));
            DrawRect(p.x - 1, p.y - 18, 3, 4, H1("#aa8020"));
        }
        else if (oid == "cupid" && p.type == "beam")
        {
            DrawRect(p.x - p.w / 2f, p.y - 1, p.w, 3, H1("#a06030"));
            DrawTri(p.x - dir * p.w / 2f, p.y - 5,
                    p.x - dir * (p.w / 2f + 8), p.y,
                    p.x - dir * p.w / 2f, p.y + 5, H1("#ffb0c8"));
            float hx = p.x + dir * p.w / 2f;
            DrawCircle(hx - dir * 3, p.y - 3, 4, H1("#ff4060"));
            DrawCircle(hx + dir * 3, p.y - 3, 4, H1("#ff4060"));
            DrawTri(hx - dir * 6, p.y - 1, hx + dir * 8, p.y + 4, hx - dir * 1, p.y + 6, H1("#ff4060"));
            Color sp = H1("#ffd0e0"); sp.a = 0.7f;
            for (int i = 0; i < 4; i++)
            {
                float tx = p.x - dir * (p.w / 2f + 6 + i * 5);
                DrawRect(tx, p.y + Mathf.Sin(frame * 0.5f + i) * 4 - 1, 2, 2, sp);
            }
        }
        else if (oid == "sam" && p.type == "ice")
        {
            DrawRect(p.x - p.w / 2f, p.y - p.h / 2f, p.w, p.h, Color.white);
            DrawRect(p.x - p.w / 2f, p.y - p.h / 2f + p.h / 3f, p.w, p.h / 3f, H1("#d42028"));
            DrawTri(p.x + dir * p.w / 2f, p.y - p.h / 2f,
                    p.x + dir * (p.w / 2f + 8), p.y,
                    p.x + dir * p.w / 2f, p.y + p.h / 2f, H1("#2a4090"));
            DrawRect(p.x - 3, p.y - 2, 5, 5, H1("#ffd040"));
            float fl = Mathf.Sin(frame * 0.5f) * 2;
            DrawTri(p.x - dir * p.w / 2f, p.y - p.h / 2f,
                    p.x - dir * (p.w / 2f + 12 + fl), p.y,
                    p.x - dir * p.w / 2f, p.y + p.h / 2f, H1("#ffaa20"));
            DrawTri(p.x - dir * p.w / 2f, p.y - p.h / 4f,
                    p.x - dir * (p.w / 2f + 6 + fl), p.y,
                    p.x - dir * p.w / 2f, p.y + p.h / 4f, H1("#ffe080"));
        }
        else if (oid == "macabee" && p.type == "poison")
        {
            float spin = frame * 0.3f;
            for (int i = 0; i < 5; i++)
            {
                float ox = Mathf.Sin(spin + i) * 5;
                float oy = Mathf.Cos(spin + i) * 5;
                Color c = (i % 2 == 1) ? H1("#2840a0") : H1("#80a0ff");
                c.a = 0.7f;
                DrawCircle(p.x + ox, p.y + oy, 8 + i, c);
            }
            DrawRect(p.x - 5, p.y - 5, 10, 10, H1("#2840a0"));
            DrawRect(p.x - 1, p.y - 8, 2, 3, H1("#e0c860"));
            DrawTri(p.x - 5, p.y + 5, p.x, p.y + 10, p.x + 5, p.y + 5, H1("#2840a0"));
            DrawRect(p.x - 2, p.y - 2, 4, 4, H1("#e0c860"));
        }
        else if (p.type == "fire")
        {
            for (int i = 0; i < 5; i++)
            {
                Color col;
                if (i == 0) col = Color.white;
                else if (i < 3) col = H1("#ffd040");
                else col = H1("#ff4020");
                DrawRect(p.x - p.w / 2f + i * 2, p.y - p.h / 2f + i, p.w - i * 4, p.h - i * 2, col);
            }
        }
        else if (p.type == "ice")
        {
            DrawRect(p.x - p.w / 2f, p.y - p.h / 2f, p.w, p.h, Color.white);
            DrawRect(p.x - p.w / 2f + 2, p.y - p.h / 2f + 2, p.w - 4, p.h - 4, H1("#5fc8ff"));
        }
        else if (p.type == "rock")
        {
            DrawRect(p.x - p.w / 2f, p.y - p.h / 2f, p.w, p.h, H1("#3a2a1a"));
            DrawRect(p.x - p.w / 2f + 3, p.y - p.h / 2f + 3, p.w - 6, p.h - 6, H1("#8a6a3a"));
        }
        else if (p.type == "beam")
        {
            Color c1 = Color.white; c1.a = 0.9f;
            DrawRect(p.x - p.w / 2f, p.y - p.h / 2f, p.w, p.h, c1);
            Color c2 = H1("#ffd040"); c2.a = 0.6f;
            DrawRect(p.x - p.w / 2f - 10, p.y - p.h, p.w + 20, p.h * 2, c2);
        }
        else if (p.type == "poison")
        {
            for (int i = 0; i < 5; i++)
            {
                float ox = Mathf.Sin(frame * 0.1f + i) * 4;
                float oy = Mathf.Cos(frame * 0.1f + i) * 4;
                Color c = (i % 2 == 1) ? H1("#3a8a2a") : H1("#80c040");
                c.a = 0.7f;
                DrawCircle(p.x + ox, p.y + oy, 8 + i, c);
            }
        }
    }

    // ===== HUD bars =====
    void DrawHUDBars()
    {
        DrawHealthBar(20, 20, fighters[0], "left");
        DrawHealthBar(W - 320, 20, fighters[1], "right");
        // Round indicators
        for (int i = 0; i < 2; i++)
        {
            Color c1 = i < roundsP1 ? H1("#ffd040") : H1("#3a2a1a");
            DrawRect(160 + i * 16, 70, 12, 12, c1);
            StrokeRect(160 + i * 16, 70, 12, 12, H1("#6a4a1a"), 1);
            Color c2 = i < roundsP2 ? H1("#ffd040") : H1("#3a2a1a");
            DrawRect(W - 160 - i * 16 - 12, 70, 12, 12, c2);
            StrokeRect(W - 160 - i * 16 - 12, 70, 12, 12, H1("#6a4a1a"), 1);
        }
        // Timer box
        DrawRect(W / 2f - 50, 14, 100, 50, H1("#1a0a25"));
        StrokeRect(W / 2f - 50, 14, 100, 50, H1("#c9a44a"), 2);

        // Cooldown bars
        DrawCdIndicator(20, 90, fighters[0]);
        DrawCdIndicator(W - 60, 90, fighters[1]);
    }

    void DrawHealthBar(float x, float y, Fighter f, string side)
    {
        float w = 300, h = 28;
        DrawRect(x - 2, y - 2, w + 4, h + 4, H1("#1a0a25"));
        StrokeRect(x - 2, y - 2, w + 4, h + 4, H1("#c9a44a"), 2);
        DrawRect(x, y, w, h, H1("#3a1a1a"));

        f.displayHp = Mathf.Max(f.hp, f.displayHp - 1.2f);
        float frac = (float)f.hp / f.hpMax;
        float trail = f.displayHp / f.hpMax;

        if (side == "left")
            DrawRect(x, y, w * trail, h, H1("#ffaa40"));
        else
            DrawRect(x + w * (1 - trail), y, w * trail, h, H1("#ffaa40"));

        Color barColor;
        if (frac > 0.6f) barColor = H1("#40dd40");
        else if (frac > 0.3f) barColor = H1("#ffdd40");
        else barColor = (frame % 20 < 10) ? H1("#ff4040") : H1("#aa2020");

        if (side == "left")
            DrawRect(x, y, w * frac, h, barColor);
        else
            DrawRect(x + w * (1 - frac), y, w * frac, h, barColor);

        for (int i = 1; i < 10; i++)
            DrawRect(x + (w / 10f) * i, y, 1, h, new Color(0, 0, 0, 0.4f));
    }

    void DrawCdIndicator(float x, float y, Fighter f)
    {
        float w = 40;
        DrawRect(x, y + 12, w, 6, H1("#1a0a25"));
        Color bar = f.cooldown > 0 ? H1("#666666") : H1("#40dd40");
        float frac = 1f - f.cooldown / 90f;
        DrawRect(x, y + 12, w * frac, 6, bar);
    }

    // ===== ARENAS =====
    void DrawArena()
    {
        switch (BACKGROUNDS[bgChoice])
        {
            case "workshop": DrawWorkshopBg(); break;
            case "graveyard": DrawGraveyardBg(); break;
            case "wonderland": DrawWonderlandBg(); break;
            case "fireworks": DrawFireworksBg(); break;
        }
    }

    void DrawWorkshopBg()
    {
        DrawVGradient(0, 0, W, GROUND, H1("#5a2a1a"), H1("#3a1a0a"));
        for (int i = 0; i < W; i += 70)
        {
            DrawRect(i, 0, 4, GROUND, H1("#3a1a0a"));
            DrawRect(i + 4, 0, 1, GROUND, H1("#4a2010"));
        }
        // Christmas tree (3 triangles for 3 tiers)
        Color treeC = H1("#1a4a2a");
        DrawTri(W / 2f - 20, GROUND - 220, W / 2f + 20, GROUND - 220, W / 2f + 60, GROUND - 100, treeC);
        DrawTri(W / 2f - 20, GROUND - 220, W / 2f - 60, GROUND - 100, W / 2f + 60, GROUND - 100, treeC);
        DrawTri(W / 2f - 50, GROUND - 160, W / 2f + 50, GROUND - 160, W / 2f + 80, GROUND, treeC);
        DrawTri(W / 2f - 50, GROUND - 160, W / 2f - 80, GROUND, W / 2f + 80, GROUND, treeC);
        // Star
        DrawCircle(W / 2f, GROUND - 222, 9, H1("#ffe040"));
        // Ornaments
        var ornaments = new (float, float, string)[] {
            (-30,-200,"#ff4040"), (20,-180,"#80c0ff"), (-10,-150,"#ffd040"),
            (30,-130,"#ff80c0"), (-40,-110,"#40dd40"), (10,-90,"#ff4040"), (-25,-60,"#ffd040")
        };
        for (int i = 0; i < ornaments.Length; i++)
        {
            var o = ornaments[i];
            DrawCircle(W / 2f + o.Item1, GROUND + o.Item2, 4, H1(o.Item3));
            if ((frame + i * 15) % 100 < 30)
                DrawRect(W / 2f + o.Item1 - 1, GROUND + o.Item2 - 1, 2, 2, Color.white);
        }
        // Presents
        for (int i = 0; i < 3; i++)
        {
            float px = 80 + i * 12;
            float py = GROUND - (i + 1) * 22;
            Color[] pcs = { H1("#cc2030"), H1("#2a8038"), H1("#2a4090") };
            DrawRect(px, py, 30 - i * 3, 20, pcs[i]);
            DrawRect(px + 12 - i, py, 4, 20, H1("#ffd040"));
            DrawRect(px, py + 8, 30 - i * 3, 4, H1("#ffd040"));
        }
        for (int i = 0; i < 2; i++)
        {
            float px2 = W - 110 - i * 8;
            float py2 = GROUND - (i + 1) * 24;
            Color[] pcs2 = { H1("#2a4090"), H1("#cc2030") };
            DrawRect(px2, py2, 32 - i * 4, 22, pcs2[i]);
            DrawRect(px2 + 14 - i, py2, 4, 22, H1("#ffd040"));
            DrawRect(px2, py2 + 9, 32 - i * 4, 4, H1("#ffd040"));
        }
        // Light strand
        for (int i = 30; i < W; i += 50)
        {
            float ly = 60 + Mathf.Sin(i * 0.04f) * 12;
            Color col;
            switch ((i / 50) % 4)
            {
                case 0: col = H1("#ff4040"); break;
                case 1: col = H1("#40dd40"); break;
                case 2: col = H1("#ffd040"); break;
                default: col = H1("#80c0ff"); break;
            }
            DrawCircle(i, ly + 5, 4, col);
            if ((frame + i) % 80 < 25)
                DrawRect(i - 1, ly + 4, 2, 2, Color.white);
        }
        // Wreath
        StrokeCircle(W / 2f, 110, 22, H1("#1a4a2a"), 8);
        DrawCircle(W / 2f - 8, 122, 3, H1("#cc2030"));
        DrawCircle(W / 2f + 8, 122, 3, H1("#cc2030"));
        // Floor
        DrawRect(0, GROUND, W, H - GROUND, H1("#3a1a0a"));
        for (int i = 0; i < W; i += 50) DrawRect(i, GROUND, 2, H - GROUND, H1("#2a0a05"));
    }

    void DrawGraveyardBg()
    {
        DrawVGradient(0, 0, W, GROUND, H1("#1a0530"), H1("#5a2080"));
        Color glow = H1("#ffe8a0"); glow.a = 0.25f;
        DrawCircle(W - 160, 110, 80, glow);
        DrawCircle(W - 160, 110, 50, H1("#fff5d0"));
        DrawRect(W - 180, 100, 8, 8, H1("#d8c890"));
        DrawRect(W - 145, 130, 6, 6, H1("#d8c890"));
        DrawRect(W - 135, 95, 4, 4, H1("#d8c890"));
        // Stars
        for (int i = 0; i < 25; i++)
        {
            float sx = (i * 79) % W;
            float sy = (i * 43) % 200;
            Color c = Color.white;
            c.a = 0.4f + Mathf.Sin(frame * 0.06f + i) * 0.4f;
            DrawRect(sx, sy, 2, 2, c);
        }
        // Bats
        for (int i = 0; i < 4; i++)
        {
            float bx = Mod(i * 200f + frame * 1.5f, W + 60f) - 30f;
            float by = 80 + Mathf.Sin(frame * 0.05f + i * 2) * 25 + i * 30;
            float wing = Mathf.Sin(frame * 0.4f + i) * 5;
            DrawRect(bx, by, 4, 3, H1("#1a0530"));
            DrawTri(bx, by + 1, bx - 9, by - wing, bx - 4, by + 2, H1("#1a0530"));
            DrawTri(bx + 4, by + 1, bx + 13, by - wing, bx + 8, by + 2, H1("#1a0530"));
        }
        // Tombstones
        for (int i = 0; i < 6; i++)
        {
            float sx = 90 + i * 130 + (i % 2) * 30;
            float sy = GROUND - 32;
            DrawRect(sx, sy, 32, 32, H1("#5a4a70"));
            if (i % 2 == 0) DrawSemiCircleTop(sx + 16, sy, 16, H1("#5a4a70"));
            else
            {
                DrawRect(sx + 14, sy - 12, 4, 14, H1("#5a4a70"));
                DrawRect(sx + 8, sy - 6, 16, 4, H1("#5a4a70"));
            }
        }
        // Foggy ground
        DrawRect(0, GROUND, W, H - GROUND, H1("#1a0530"));
        for (int i = 0; i < 12; i++)
        {
            float fx = Mod(i * 110f + frame * 0.6f, W + 80f) - 40f;
            Color fc = H1("#3a1850"); fc.a = 0.5f;
            DrawEllipse(fx, GROUND + 12, 50, 7, fc);
        }
        // Pumpkins
        int[] pxs = { 220, 470, 760 };
        foreach (var px in pxs)
        {
            DrawEllipse(px, GROUND + 18, 14, 11, H1("#ff7820"));
            DrawRect(px - 1, GROUND + 4, 2, 4, H1("#3a6020"));
            DrawRect(px - 6, GROUND + 14, 3, 2, H1("#ffe040"));
            DrawRect(px + 3, GROUND + 14, 3, 2, H1("#ffe040"));
            DrawRect(px - 4, GROUND + 19, 8, 2, H1("#ffe040"));
        }
    }

    void DrawWonderlandBg()
    {
        DrawVGradient(0, 0, W, GROUND, H1("#1a3a5a"), H1("#5a8aaa"));
        for (int i = 0; i < 80; i++)
        {
            float x = Mod(i * 53f + frame * 0.4f, W);
            float y = Mod(i * 31f + frame * 0.8f, GROUND);
            Color c = H1("#e0f0ff"); c.a = 0.7f;
            DrawRect(x, y, 2, 2, c);
        }
        // Mountains as series of triangles
        Color mt = H1("#c8d8e8");
        DrawTri(0, GROUND, 150, 200, 350, 160, mt);
        DrawTri(0, GROUND, 350, 160, 350, GROUND, mt);
        DrawTri(350, GROUND, 350, 160, 550, 220, mt);
        DrawTri(350, GROUND, 550, 220, 550, GROUND, mt);
        DrawTri(550, GROUND, 550, 220, 750, 180, mt);
        DrawTri(550, GROUND, 750, 180, 750, GROUND, mt);
        DrawTri(750, GROUND, 750, 180, W, 240, mt);
        DrawTri(750, GROUND, W, 240, W, GROUND, mt);
        // Snow caps
        DrawTri(120, 220, 150, 200, 180, 220, Color.white);
        DrawTri(320, 180, 350, 160, 380, 180, Color.white);
        // Pine trees
        int[] txs = { 400, 480, 600, 700 };
        foreach (var tx in txs)
        {
            DrawTri(tx - 14, GROUND - 5, tx, GROUND - 70, tx + 14, GROUND - 5, H1("#1a4a2a"));
            Color sc = Color.white; sc.a = 0.6f;
            DrawTri(tx - 12, GROUND - 8, tx, GROUND - 60, tx + 12, GROUND - 8, sc);
        }
        // Snowman
        int smx = 150;
        DrawCircle(smx, GROUND - 8, 16, Color.white);
        DrawCircle(smx, GROUND - 30, 12, Color.white);
        DrawCircle(smx, GROUND - 48, 9, Color.white);
        DrawTri(smx, GROUND - 48, smx + 9, GROUND - 47, smx, GROUND - 46, H1("#ff8020"));
        DrawRect(smx - 4, GROUND - 51, 2, 2, H1("#1a0a05"));
        DrawRect(smx + 2, GROUND - 51, 2, 2, H1("#1a0a05"));
        DrawRect(smx - 1, GROUND - 34, 2, 2, H1("#1a0a05"));
        DrawRect(smx - 1, GROUND - 30, 2, 2, H1("#1a0a05"));
        DrawRect(smx - 1, GROUND - 26, 2, 2, H1("#1a0a05"));
        DrawRect(smx - 9, GROUND - 56, 18, 2, H1("#1a0a05"));
        DrawRect(smx - 6, GROUND - 66, 12, 10, H1("#1a0a05"));
        DrawLine(smx - 12, GROUND - 28, smx - 22, GROUND - 38, H1("#5a3010"), 2);
        DrawLine(smx - 22, GROUND - 38, smx - 26, GROUND - 32, H1("#5a3010"), 2);
        DrawLine(smx + 12, GROUND - 28, smx + 22, GROUND - 38, H1("#5a3010"), 2);
        DrawLine(smx + 22, GROUND - 38, smx + 26, GROUND - 32, H1("#5a3010"), 2);
        // Light strand
        for (int i = 30; i < W; i += 55)
        {
            float ly = 60 + Mathf.Sin(i * 0.05f) * 12;
            Color ccol;
            switch ((i / 55) % 5)
            {
                case 0: ccol = H1("#ff4040"); break;
                case 1: ccol = H1("#40dd40"); break;
                case 2: ccol = H1("#ffd040"); break;
                case 3: ccol = H1("#80c0ff"); break;
                default: ccol = H1("#ff80c0"); break;
            }
            DrawCircle(i, ly + 5, 4, ccol);
        }
        DrawRect(0, GROUND, W, H - GROUND, H1("#a8d0e8"));
        for (int i = 0; i < W; i += 40) DrawRect(i, GROUND + 20, 20, 2, H1("#80b0d0"));
    }

    void DrawFireworksBg()
    {
        DrawVGradient(0, 0, W, GROUND, H1("#0a0a30"), H1("#5a3080"));
        for (int i = 0; i < 30; i++)
        {
            float sx = (i * 73) % W;
            float sy = (i * 41) % 150;
            Color c = Color.white;
            c.a = 0.5f + Mathf.Sin(frame * 0.05f + i) * 0.3f;
            DrawRect(sx, sy, 2, 2, c);
        }
        // Bunting
        for (int i = 0; i < W; i += 50)
        {
            Color col;
            switch ((i / 50) % 3)
            {
                case 0: col = H1("#d42028"); break;
                case 1: col = Color.white; break;
                default: col = H1("#2a4090"); break;
            }
            DrawTri(i, 25, i + 30, 25, i + 15, 48, col);
        }
        // City silhouette
        for (int i = 0; i < 12; i++)
        {
            float x = i * 82 + 20;
            float h = 100 + (i * 37) % 80;
            DrawRect(x, GROUND - h, 60, h, H1("#0a0510"));
            float wy = GROUND - h + 10;
            while (wy < GROUND - 10)
            {
                float wx = x + 8;
                while (wx < x + 52)
                {
                    if ((((int)(wx + wy)) + i) % 31 < 12)
                        DrawRect(wx, wy, 4, 6, H1("#ffd040"));
                    wx += 12;
                }
                wy += 16;
            }
        }
        // Fireworks bursts
        var bursts = new (int, int, string, int)[] {
            (200, 170, "#ff4040", 0), (500, 110, "#80c0ff", 35),
            (800, 140, "#ffd040", 70), (350, 90, "#ffffff", 105),
            (700, 200, "#40dd40", 140), (120, 100, "#ff80c0", 175)
        };
        foreach (var b in bursts)
        {
            int cycle = (frame + b.Item4) % 200;
            if (cycle < 35)
            {
                float r = cycle * 2.8f;
                float fade = 1f - cycle / 35f;
                float cr = Mathf.Max(0f, 12f - cycle / 3f);
                if (cr > 0)
                {
                    Color fc = Color.white; fc.a = fade;
                    DrawCircle(b.Item1, b.Item2, cr, fc);
                }
                Color sparkColor = H1(b.Item3); sparkColor.a = fade;
                for (int aIdx = 0; aIdx < 12; aIdx++)
                {
                    float ang = aIdx * Mathf.PI / 6f;
                    DrawLine(b.Item1 + Mathf.Cos(ang) * (r * 0.4f), b.Item2 + Mathf.Sin(ang) * (r * 0.4f),
                             b.Item1 + Mathf.Cos(ang) * r,          b.Item2 + Mathf.Sin(ang) * r,
                             sparkColor, 2);
                }
                for (int aIdx = 0; aIdx < 16; aIdx++)
                {
                    float ang2 = aIdx * Mathf.PI / 8f + cycle * 0.05f;
                    float er = r * 0.85f;
                    DrawRect(b.Item1 + Mathf.Cos(ang2) * er - 1, b.Item2 + Mathf.Sin(ang2) * er - 1, 2, 2, sparkColor);
                }
            }
        }
        DrawRect(0, GROUND, W, H - GROUND, H1("#2a2030"));
        for (int i = 0; i < W; i += 40) DrawRect(i, GROUND, 2, H - GROUND, H1("#1a1020"));
    }

    // ===== FIGHTER SPRITE (procedural drawing with manual transform) =====
    // Local-to-world transform helper struct
    struct LT
    {
        public float ox, oy;
        public float rotation;
        public float sx, sy;
        public Vector2 P(float lx, float ly)
        {
            float x = lx * sx;
            float y = ly * sy;
            if (rotation != 0)
            {
                float c = Mathf.Cos(rotation);
                float s = Mathf.Sin(rotation);
                float rx = x * c - y * s;
                float ry = x * s + y * c;
                x = rx; y = ry;
            }
            return new Vector2(ox + x, oy + y);
        }
    }

    void DrawFighterSprite(float cx, float cy, CharDef chr, int facing, string fState,
        int stateTimer, float scale, Attack activeAttack, float walkPhase, bool flashWhite, bool poisoned)
    {
        int dir = facing;
        Color primary = flashWhite ? Color.white : H1(chr.primaryHex);
        Color accent  = flashWhite ? Color.white : H1(chr.accentHex);
        Color skin    = flashWhite ? Color.white : H1(chr.skinHex);
        Color dark    = flashWhite ? new Color(0.8f, 0.8f, 0.8f) : H1("#1a0a10");

        float BW, BH, HEAD_R;
        switch (chr.build)
        {
            case "huge":  BW = 36; BH = 50; HEAD_R = 18; break;
            case "broad": BW = 30; BH = 44; HEAD_R = 14; break;
            case "small": BW = 20; BH = 38; HEAD_R = 11; break;
            default:      BW = 24; BH = 44; HEAD_R = 14; break;
        }
        BW *= scale; BH *= scale; HEAD_R *= scale;
        float ARM_W = 8 * scale, ARM_L = 22 * scale;
        float LEG_W = 10 * scale, LEG_L = 28 * scale;

        float crouchOffset = 0, armSwingL = 0, armSwingR = 0;
        float legSwingL = 0, legSwingR = 0;
        float bodyTilt = 0, punchExtend = 0, kickExtend = 0;

        if (fState == "walk")
        {
            legSwingL = Mathf.Sin(walkPhase) * 8 * scale;
            legSwingR = -legSwingL;
            armSwingL = -legSwingL * 0.5f;
            armSwingR = legSwingL * 0.5f;
        }
        else if (fState == "crouch") crouchOffset = 18 * scale;
        else if (fState == "jump") { armSwingL = -12 * scale; armSwingR = 12 * scale; legSwingL = -8 * scale; legSwingR = -8 * scale; }
        else if (fState == "fall") { armSwingL = -6 * scale; armSwingR = 6 * scale; }
        else if (fState == "punch" && activeAttack != null)
        {
            float t = 1f - stateTimer / (float)activeAttack.frames;
            float ex = Mathf.Sin(t * Mathf.PI);
            punchExtend = ex * 28 * scale;
            bodyTilt = ex * 0.1f;
        }
        else if (fState == "kick" && activeAttack != null)
        {
            float t = 1f - stateTimer / (float)activeAttack.frames;
            float ex = Mathf.Sin(t * Mathf.PI);
            kickExtend = ex * 38 * scale;
            bodyTilt = -ex * 0.05f;
        }
        else if (fState == "block") { armSwingL = 6 * scale; armSwingR = -6 * scale; }
        else if (fState == "hit") { bodyTilt = -dir * 0.15f; armSwingL = -10 * scale; armSwingR = 10 * scale; }
        else if (fState == "ko")
        {
            DrawKO(cx, cy, chr);
            return;
        }

        var t2 = new LT { ox = cx, oy = cy - crouchOffset, rotation = bodyTilt * dir, sx = dir, sy = 1 };

        // === Legs ===
        DrawLocalRect(t2, -LEG_W - 3, -LEG_L + legSwingR, LEG_W, LEG_L, dark);
        DrawLocalRect(t2, -LEG_W - 3 + 1, -LEG_L + legSwingR + 1, LEG_W - 2, LEG_L - 4, primary);
        DrawLocalRect(t2, -LEG_W - 5, -2 + legSwingR, LEG_W + 4, 4, dark);
        DrawLocalRect(t2, 3, -LEG_L + legSwingL, LEG_W, LEG_L, dark);
        DrawLocalRect(t2, 3 + 1, -LEG_L + legSwingL + 1, LEG_W - 2, LEG_L - 4, primary);
        DrawLocalRect(t2, 1, -2 + legSwingL, LEG_W + 4, 4, dark);
        if (kickExtend > 0)
        {
            DrawLocalRect(t2, LEG_W, -LEG_L * 0.4f, kickExtend, LEG_W, dark);
            DrawLocalRect(t2, LEG_W, -LEG_L * 0.4f + 1, kickExtend, LEG_W - 2, primary);
            DrawLocalRect(t2, LEG_W + kickExtend, -LEG_L * 0.4f - 2, 6, LEG_W + 4, dark);
        }

        // === Body ===
        DrawLocalRect(t2, -BW / 2 - 1, -LEG_L - BH, BW + 2, BH + 2, dark);
        DrawLocalRect(t2, -BW / 2, -LEG_L - BH + 1, BW, BH, primary);
        DrawLocalRect(t2, -BW / 2, -LEG_L - 10, BW, 4, accent);
        // Chest detail
        DrawChestDetail(t2, chr, BW, BH, LEG_L);

        // === Arms ===
        DrawLocalRect(t2, -BW / 2 - ARM_W + 1, -LEG_L - BH + 4 + armSwingL, ARM_W, ARM_L, dark);
        DrawLocalRect(t2, -BW / 2 - ARM_W + 2, -LEG_L - BH + 5 + armSwingL, ARM_W - 2, ARM_L - 2, primary);
        DrawLocalRect(t2, -BW / 2 - ARM_W, -LEG_L - BH + ARM_L + armSwingL, ARM_W + 2, 8, accent);

        if (punchExtend > 0)
        {
            DrawLocalRect(t2, BW / 2 - 1, -LEG_L - BH + 12, ARM_L + punchExtend, ARM_W, dark);
            DrawLocalRect(t2, BW / 2, -LEG_L - BH + 13, ARM_L + punchExtend, ARM_W - 2, primary);
            DrawLocalRect(t2, BW / 2 + ARM_L + punchExtend - 2, -LEG_L - BH + 9, 10, 12, accent);
            DrawLocalRect(t2, BW / 2 + ARM_L + punchExtend, -LEG_L - BH + 11, 8, 8, dark);
            Color sl = Color.white; sl.a = 0.6f;
            for (int i = 0; i < 3; i++)
                DrawLocalRect(t2, BW / 2 - 5 - i * 4, -LEG_L - BH + 14 + i * 2, 6, 1, sl);
        }
        else
        {
            DrawLocalRect(t2, BW / 2 - 1, -LEG_L - BH + 4 + armSwingR, ARM_W, ARM_L, dark);
            DrawLocalRect(t2, BW / 2, -LEG_L - BH + 5 + armSwingR, ARM_W - 2, ARM_L - 2, primary);
            DrawLocalRect(t2, BW / 2 - 1, -LEG_L - BH + ARM_L + armSwingR, ARM_W + 2, 8, accent);
        }

        // Special-move aura
        if (fState == "special" && stateTimer > 0 && activeAttack != null)
        {
            Color aura = AuraColor(chr.id);
            aura.a = 0.6f + Mathf.Sin(frame * 0.4f) * 0.3f;
            DrawLocalRect(t2, BW / 2 + ARM_L - 4, -LEG_L - BH + 8, 14, 14, aura);
            aura.a = 0.3f;
            DrawLocalRect(t2, BW / 2 + ARM_L - 8, -LEG_L - BH + 4, 22, 22, aura);
        }

        // === Head ===
        float headY = -LEG_L - BH - HEAD_R + 2;
        DrawLocalCircle(t2, 0, headY, HEAD_R + 1, dark);
        DrawLocalCircle(t2, 0, headY, HEAD_R, skin);
        DrawCharacterFace(t2, chr, headY, HEAD_R);

        // Block shield (drawn in world after transform)
        if (fState == "block")
        {
            Color sh = H1("#80c0ff"); sh.a = 0.4f + Mathf.Sin(frame * 0.4f) * 0.2f;
            var bp = t2.P(BW / 2 + 4, -LEG_L - BH / 2);
            DrawCircle(bp.x, bp.y, BH * 0.4f, sh);
            for (int i = 0; i < 4; i++)
            {
                var sp = t2.P(BW / 2 + 8 + Mathf.Sin(frame * 0.3f + i) * 4,
                              -LEG_L - BH / 2 + Mathf.Cos(frame * 0.3f + i) * BH * 0.5f);
                DrawRect(sp.x, sp.y, 2, 2, Color.white);
            }
        }

        // Poison aura
        if (poisoned)
        {
            Color pc = H1("#80ff40"); pc.a = 0.3f;
            for (int i = 0; i < 5; i++)
            {
                float a = frame * 0.05f + i * 1.2f;
                DrawRect(cx + Mathf.Cos(a) * 30 - 2, cy - 50 + Mathf.Sin(a) * 40 - 2, 4, 4, pc);
            }
        }
    }

    Color AuraColor(string id)
    {
        switch (id)
        {
            case "santa": return H1("#ff4040");
            case "easter": return H1("#fff080");
            case "jack": return H1("#a040ff");
            case "turkey": return H1("#e0a428");
            case "cupid": return H1("#ff80a0");
            case "patty": return H1("#40dd40");
            case "sam": return H1("#80a0ff");
            case "macabee": return H1("#e0c860");
            default: return Color.white;
        }
    }

    void DrawKO(float cx, float cy, CharDef chr)
    {
        DrawRect(cx - 40, cy - 16, 80, 14, H1(chr.primaryHex));
        DrawRect(cx - 40, cy - 16, 80, 3, H1(chr.accentHex));
        DrawCircle(cx - 30, cy - 8, 10, H1(chr.skinHex));
        DrawLine(cx - 34, cy - 12, cx - 26, cy - 4, Color.black, 2);
        DrawLine(cx - 26, cy - 12, cx - 34, cy - 4, Color.black, 2);
        for (int i = 0; i < 3; i++)
        {
            float a = frame * 0.05f + i * Mathf.PI * 2f / 3f;
            DrawRect(cx - 30 + Mathf.Cos(a) * 16 - 2, cy - 25 + Mathf.Sin(a) * 8 - 2, 4, 4, H1("#ffd040"));
        }
    }

    // ===== Chest details =====
    void DrawChestDetail(LT t, CharDef chr, float BW, float BH, float LEG_L)
    {
        float cy = -LEG_L - BH / 2f;
        switch (chr.id)
        {
            case "santa":
                DrawLocalRect(t, -BW / 2, cy + 6, BW, 7, H1("#1a0a05"));
                DrawLocalRect(t, -5, cy + 7, 10, 5, H1("#ffd040"));
                DrawLocalRect(t, -2, cy + 8, 4, 3, H1("#1a0a05"));
                DrawLocalRect(t, -2, cy - BH / 3, 4, BH / 2, Color.white);
                break;
            case "easter":
                DrawLocalEllipse(t, 0, cy, 5, 7, H1("#fff080"));
                DrawLocalRect(t, -4, cy - 1, 8, 2, H1("#80c0e8"));
                DrawLocalRect(t, -3, cy + 3, 6, 1, H1("#80c0e8"));
                DrawLocalRect(t, -2, cy - 4, 4, 1, H1("#ff80a0"));
                break;
            case "jack":
                // Bat - simple polygon as triangles
                DrawLocalTri(t, -9, cy, -5, cy - 4, -1, cy, H1("#1a0a25"));
                DrawLocalTri(t, -1, cy, 0, cy - 1, 1, cy, H1("#1a0a25"));
                DrawLocalTri(t, 1, cy, 5, cy - 4, 9, cy, H1("#1a0a25"));
                DrawLocalTri(t, -9, cy, -1, cy, -7, cy + 3, H1("#1a0a25"));
                DrawLocalTri(t, -7, cy + 3, -1, cy, 0, cy + 4, H1("#1a0a25"));
                DrawLocalTri(t, 0, cy + 4, 1, cy, 7, cy + 3, H1("#1a0a25"));
                DrawLocalTri(t, 1, cy, 9, cy, 7, cy + 3, H1("#1a0a25"));
                break;
            case "turkey":
                for (int i = -1; i <= 1; i++) DrawLocalCircle(t, i * 4, cy, 2, H1("#aa2030"));
                DrawLocalRect(t, -7, cy - 4, 4, 4, H1("#3a8030"));
                DrawLocalRect(t, 3, cy - 4, 4, 4, H1("#3a8030"));
                break;
            case "cupid":
                DrawLocalCircle(t, -3, cy, 3, H1("#ff4060"));
                DrawLocalCircle(t, 3, cy, 3, H1("#ff4060"));
                DrawLocalTri(t, -6, cy + 1, 0, cy + 7, 6, cy + 1, H1("#ff4060"));
                break;
            case "patty":
                DrawLocalCircle(t, -4, cy - 2, 3, H1("#3aa040"));
                DrawLocalCircle(t, 4, cy - 2, 3, H1("#3aa040"));
                DrawLocalCircle(t, 0, cy + 2, 3, H1("#3aa040"));
                DrawLocalRect(t, -1, cy + 3, 2, 5, H1("#1a4020"));
                break;
            case "sam":
                for (int i = 0; i < 3; i++)
                {
                    DrawLocalRect(t, -BW / 2, cy - 6 + i * 4, BW, 2, Color.white);
                    DrawLocalRect(t, -BW / 2, cy - 4 + i * 4, BW, 2, H1("#d42028"));
                }
                // Star (5-pointed approximated as 5 triangles fanning from center)
                DrawLocalCircle(t, 0, cy + 9, 4, H1("#ffd040"));
                break;
            case "macabee":
                DrawLocalTri(t, 0, cy - 6, -6, cy + 4, 6, cy + 4, H1(chr.accentHex));
                DrawLocalTri(t, 0, cy + 6, -6, cy - 4, 6, cy - 4, H1(chr.accentHex));
                break;
        }
    }

    // ===== Character faces =====
    void DrawCharacterFace(LT t, CharDef chr, float headY, float R)
    {
        float eyeY = headY - 1;
        float mouthY = headY + R * 0.4f;

        switch (chr.id)
        {
            case "santa":
                DrawLocalTri(t, -R, headY - R + 4, R, headY - R + 4, R - 6, headY - R - 16, H1(chr.primaryHex));
                DrawLocalRect(t, -R, headY - R, R * 2, 5, Color.white);
                DrawLocalCircle(t, R - 6, headY - R - 16, 4, Color.white);
                DrawLocalRect(t, -5, eyeY, 2, 2, Color.black);
                DrawLocalRect(t, 3, eyeY, 2, 2, Color.black);
                Color ch = H1("#ffaa90"); ch.a = 0.7f;
                DrawLocalRect(t, -R + 1, eyeY + 4, 4, 3, ch);
                DrawLocalRect(t, R - 5, eyeY + 4, 4, 3, ch);
                DrawLocalLowerBeard(t, headY + 4, R, Color.white);
                DrawLocalRect(t, -R, headY + 2, R * 2, R - 2, Color.white);
                DrawLocalCircle(t, -R + 2, headY + R - 2, 3, Color.white);
                DrawLocalCircle(t, R - 2, headY + R - 2, 3, Color.white);
                DrawLocalCircle(t, 0, headY + 2, 3, H1("#ff5050"));
                DrawLocalRect(t, -7, headY + 5, 14, 2, Color.white);
                break;
            case "easter":
                DrawLocalRect(t, -8, headY - R - 22, 5, 26, H1(chr.primaryHex));
                DrawLocalRect(t, 3, headY - R - 22, 5, 26, H1(chr.primaryHex));
                DrawLocalRect(t, -7, headY - R - 20, 3, 20, H1("#ffd0e0"));
                DrawLocalRect(t, 4, headY - R - 20, 3, 20, H1("#ffd0e0"));
                DrawLocalTri(t, 0, headY + 5, -3, headY + 1, 3, headY + 1, H1("#ff60a0"));
                DrawLocalRect(t, -7, eyeY - 1, 4, 4, Color.black);
                DrawLocalRect(t, 3, eyeY - 1, 4, 4, Color.black);
                DrawLocalRect(t, -6, eyeY - 1, 2, 2, Color.white);
                DrawLocalRect(t, 4, eyeY - 1, 2, 2, Color.white);
                DrawLocalRect(t, -3, mouthY + 3, 2, 5, Color.white);
                DrawLocalRect(t, 1, mouthY + 3, 2, 5, Color.white);
                DrawLocalLine(t, -3, mouthY + 1, -10, mouthY, Color.black, 1);
                DrawLocalLine(t, -3, mouthY + 2, -10, mouthY + 3, Color.black, 1);
                DrawLocalLine(t, 3, mouthY + 1, 10, mouthY, Color.black, 1);
                DrawLocalLine(t, 3, mouthY + 2, 10, mouthY + 3, Color.black, 1);
                break;
            case "jack":
                // Pumpkin ridges (simplified - lines instead of beziers)
                for (int i = -2; i <= 2; i++)
                {
                    float xc = i * 5f;
                    DrawLocalLine(t, xc, headY - R + 2, xc * 0.7f, headY, H1("#aa4010"), 2);
                    DrawLocalLine(t, xc * 0.7f, headY, xc, headY + R - 2, H1("#aa4010"), 2);
                }
                DrawLocalRect(t, -3, headY - R - 5, 6, 7, H1("#3a6020"));
                DrawLocalRect(t, -2, headY - R - 5, 2, 7, H1("#5a8030"));
                DrawLocalTri(t, -9, eyeY - 2, -3, eyeY - 2, -6, eyeY + 4, H1("#ffe040"));
                DrawLocalTri(t, 9, eyeY - 2, 3, eyeY - 2, 6, eyeY + 4, H1("#ffe040"));
                Color spark = Color.white; spark.a = 0.7f + Mathf.Sin(frame * 0.3f) * 0.3f;
                DrawLocalRect(t, -7, eyeY, 2, 1, spark);
                DrawLocalRect(t, 5, eyeY, 2, 1, spark);
                // Jagged grin (zig-zag rect bottom)
                DrawLocalRect(t, -9, mouthY + 1, 18, 5, H1("#ffe040"));
                break;
            case "turkey":
                DrawLocalRect(t, -R, headY - R, R * 2, R + 2, H1("#5a2a10"));
                DrawLocalRect(t, -R + 2, headY - R, 2, R, H1("#3a1a05"));
                DrawLocalRect(t, R - 4, headY - R, 2, R, H1("#3a1a05"));
                for (int i = -1; i <= 1; i++)
                    DrawLocalTri(t, i * 5 - 2, headY - R + 2, i * 5, headY - R - 6, i * 5 + 2, headY - R + 2, H1(chr.accentHex));
                DrawLocalTri(t, 0, headY + 1, R + 5, headY + 4, 0, headY + 6, H1("#ffc040"));
                DrawLocalRect(t, 0, headY + 4, 4, 1, H1("#aa7020"));
                DrawLocalTri(t, R - 3, headY + 2, R + 2, headY + 9, R - 7, headY + 9, H1("#cc2030"));
                DrawLocalRect(t, -2, eyeY - 1, 5, 5, Color.white);
                DrawLocalRect(t, 0, eyeY, 3, 3, Color.black);
                DrawLocalRect(t, 1, eyeY + 1, 1, 1, Color.white);
                break;
            case "cupid":
                for (int i = -2; i <= 2; i++) DrawLocalCircle(t, i * 4, headY - R + 2, 4, H1("#ffd040"));
                DrawLocalRect(t, -R, headY - R + 2, R * 2, 3, H1("#ffd040"));
                DrawLocalCircle(t, -R, headY - R + 8, 3, H1("#ffd040"));
                DrawLocalCircle(t, R, headY - R + 8, 3, H1("#ffd040"));
                DrawLocalEllipseOutline(t, 0, headY - R - 5, 9, 3, H1("#ffe080"), 2);
                DrawLocalRect(t, -7, eyeY - 1, 5, 5, Color.white);
                DrawLocalRect(t, 2, eyeY - 1, 5, 5, Color.white);
                DrawLocalRect(t, -5, eyeY, 2, 3, H1("#2a4080"));
                DrawLocalRect(t, 4, eyeY, 2, 3, H1("#2a4080"));
                DrawLocalRect(t, -4, eyeY, 1, 1, Color.white);
                DrawLocalRect(t, 5, eyeY, 1, 1, Color.white);
                Color cc = H1("#ffaab8"); cc.a = 0.7f;
                DrawLocalRect(t, -9, mouthY - 1, 3, 2, cc);
                DrawLocalRect(t, 6, mouthY - 1, 3, 2, cc);
                DrawLocalRect(t, -2, mouthY + 1, 4, 1, H1("#aa4060"));
                DrawLocalRect(t, -3, mouthY, 1, 1, H1("#aa4060"));
                DrawLocalRect(t, 2, mouthY, 1, 1, H1("#aa4060"));
                break;
            case "patty":
                DrawLocalRect(t, -R - 3, headY - R, R * 2 + 6, 3, H1(chr.primaryHex));
                DrawLocalRect(t, -R + 1, headY - R - 14, R * 2 - 2, 14, H1(chr.primaryHex));
                DrawLocalRect(t, -R + 1, headY - R - 5, R * 2 - 2, 4, H1(chr.accentHex));
                DrawLocalRect(t, -2, headY - R - 4, 4, 2, H1("#1a0a05"));
                DrawLocalRect(t, R - 4, headY - R - 12, 3, 3, H1("#1a4020"));
                DrawLocalRect(t, -5, eyeY, 3, 2, Color.black);
                DrawLocalRect(t, 2, eyeY, 3, 2, Color.black);
                DrawLocalLowerBeard(t, headY + 6, R, H1("#cc4020"));
                DrawLocalRect(t, -R, headY + 4, R * 2, R, H1("#cc4020"));
                DrawLocalRect(t, -R + 2, headY + 8, 2, 3, H1("#aa3010"));
                DrawLocalRect(t, R - 4, headY + 8, 2, 3, H1("#aa3010"));
                DrawLocalRect(t, -7, headY + 4, 14, 2, H1("#cc4020"));
                break;
            case "sam":
                DrawLocalRect(t, -R - 2, headY - R + 2, R * 2 + 4, 3, H1(chr.primaryHex));
                DrawLocalRect(t, -R + 1, headY - R - 16, R * 2 - 2, 16, H1(chr.primaryHex));
                DrawLocalRect(t, -R + 1, headY - R - 10, R * 2 - 2, 4, Color.white);
                DrawLocalRect(t, -R + 1, headY - R - 10, R * 2 - 2, 1, H1("#d42028"));
                DrawLocalRect(t, -R + 1, headY - R - 7, R * 2 - 2, 1, H1("#d42028"));
                for (int i = -1; i <= 1; i++) DrawLocalRect(t, i * 5 - 1, headY - R - 9, 2, 2, H1("#2a4090"));
                DrawLocalRect(t, -7, eyeY, 4, 2, Color.black);
                DrawLocalRect(t, 3, eyeY, 4, 2, Color.black);
                DrawLocalRect(t, -8, eyeY - 3, 5, 2, Color.white);
                DrawLocalRect(t, 3, eyeY - 3, 5, 2, Color.white);
                DrawLocalTri(t, -5, mouthY, 0, headY + R + 4, 5, mouthY, Color.white);
                DrawLocalRect(t, -6, mouthY - 1, 12, 2, Color.white);
                DrawLocalRect(t, -7, mouthY - 2, 2, 1, Color.white);
                DrawLocalRect(t, 5, mouthY - 2, 2, 1, Color.white);
                break;
            case "macabee":
                DrawLocalTri(t, -R, headY - R + 4, 0, headY - R - 12, R, headY - R + 4, H1(chr.primaryHex));
                DrawLocalRect(t, -R, headY - R + 2, R * 2, 3, H1(chr.accentHex));
                DrawLocalRect(t, -1, headY - R - 7, 2, 6, H1(chr.accentHex));
                DrawLocalRect(t, -3, headY - R - 5, 6, 2, H1(chr.accentHex));
                DrawLocalRect(t, -7, eyeY, 4, 3, Color.white);
                DrawLocalRect(t, 3, eyeY, 4, 3, Color.white);
                DrawLocalRect(t, -6, eyeY + 1, 2, 2, H1("#1a0a05"));
                DrawLocalRect(t, 4, eyeY + 1, 2, 2, H1("#1a0a05"));
                DrawLocalLowerBeard(t, headY + 4, R - 1, H1("#1a0a05"));
                DrawLocalRect(t, -R + 2, headY + 2, R * 2 - 4, R, H1("#1a0a05"));
                DrawLocalRect(t, -5, headY + 3, 10, 2, H1("#1a0a05"));
                break;
        }
    }

    // ===== Local-space drawing helpers (apply LT then draw) =====
    void DrawLocalRect(LT t, float lx, float ly, float lw, float lh, Color c)
    {
        // 4 corners
        Vector2 p1 = t.P(lx, ly);
        Vector2 p2 = t.P(lx + lw, ly);
        Vector2 p3 = t.P(lx + lw, ly + lh);
        Vector2 p4 = t.P(lx, ly + lh);
        DrawQuad(p1, p2, p3, p4, c);
    }
    void DrawLocalCircle(LT t, float lx, float ly, float r, Color c)
    {
        Vector2 p = t.P(lx, ly);
        DrawCircle(p.x, p.y, r * Mathf.Abs(t.sx), c);
    }
    void DrawLocalEllipse(LT t, float lx, float ly, float rx, float ry, Color c)
    {
        Vector2 p = t.P(lx, ly);
        DrawEllipse(p.x, p.y, rx, ry, c);
    }
    void DrawLocalEllipseOutline(LT t, float lx, float ly, float rx, float ry, Color c, float w)
    {
        Vector2 p = t.P(lx, ly);
        DrawEllipseOutline(p.x, p.y, rx, ry, c, w);
    }
    void DrawLocalTri(LT t, float ax, float ay, float bx, float by, float cx, float cy, Color c)
    {
        Vector2 a = t.P(ax, ay);
        Vector2 b = t.P(bx, by);
        Vector2 d = t.P(cx, cy);
        DrawTri(a.x, a.y, b.x, b.y, d.x, d.y, c);
    }
    void DrawLocalLine(LT t, float ax, float ay, float bx, float by, Color c, float w)
    {
        Vector2 a = t.P(ax, ay);
        Vector2 b = t.P(bx, by);
        DrawLine(a.x, a.y, b.x, b.y, c, w);
    }
    void DrawLocalLowerBeard(LT t, float ly, float r, Color c)
    {
        // Half circle below the face center
        int seg = 16;
        Vector2 prev = t.P(r, ly);
        for (int i = 1; i <= seg; i++)
        {
            float ang = i * Mathf.PI / seg;
            Vector2 curr = t.P(Mathf.Cos(ang) * r, ly + Mathf.Sin(ang) * r);
            // Triangulate against center
            Vector2 center = t.P(0, ly);
            DrawTri(center.x, center.y, prev.x, prev.y, curr.x, curr.y, c);
            prev = curr;
        }
    }

    // ===== Low-level GL drawing primitives =====
    void DrawRect(float x, float y, float w, float h, Color c)
    {
        GL.Begin(GL.TRIANGLES);
        GL.Color(c);
        GL.Vertex3(x, y, 0);
        GL.Vertex3(x + w, y, 0);
        GL.Vertex3(x + w, y + h, 0);
        GL.Vertex3(x, y, 0);
        GL.Vertex3(x + w, y + h, 0);
        GL.Vertex3(x, y + h, 0);
        GL.End();
    }

    void DrawQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color col)
    {
        GL.Begin(GL.TRIANGLES);
        GL.Color(col);
        GL.Vertex3(a.x, a.y, 0); GL.Vertex3(b.x, b.y, 0); GL.Vertex3(c.x, c.y, 0);
        GL.Vertex3(a.x, a.y, 0); GL.Vertex3(c.x, c.y, 0); GL.Vertex3(d.x, d.y, 0);
        GL.End();
    }

    void DrawTri(float ax, float ay, float bx, float by, float cx, float cy, Color col)
    {
        GL.Begin(GL.TRIANGLES);
        GL.Color(col);
        GL.Vertex3(ax, ay, 0);
        GL.Vertex3(bx, by, 0);
        GL.Vertex3(cx, cy, 0);
        GL.End();
    }

    void DrawCircle(float cx, float cy, float r, Color col, int segs = 18)
    {
        if (r <= 0) return;
        GL.Begin(GL.TRIANGLES);
        GL.Color(col);
        for (int i = 0; i < segs; i++)
        {
            float a1 = i * 2f * Mathf.PI / segs;
            float a2 = (i + 1) * 2f * Mathf.PI / segs;
            GL.Vertex3(cx, cy, 0);
            GL.Vertex3(cx + Mathf.Cos(a1) * r, cy + Mathf.Sin(a1) * r, 0);
            GL.Vertex3(cx + Mathf.Cos(a2) * r, cy + Mathf.Sin(a2) * r, 0);
        }
        GL.End();
    }

    void StrokeCircle(float cx, float cy, float r, Color col, float w, int segs = 32)
    {
        // Approximate stroke with a band of small triangles
        float ri = r - w / 2f;
        float ro = r + w / 2f;
        if (ri < 0) ri = 0;
        GL.Begin(GL.TRIANGLES);
        GL.Color(col);
        for (int i = 0; i < segs; i++)
        {
            float a1 = i * 2f * Mathf.PI / segs;
            float a2 = (i + 1) * 2f * Mathf.PI / segs;
            float c1x = Mathf.Cos(a1), s1y = Mathf.Sin(a1);
            float c2x = Mathf.Cos(a2), s2y = Mathf.Sin(a2);
            GL.Vertex3(cx + c1x * ri, cy + s1y * ri, 0);
            GL.Vertex3(cx + c2x * ri, cy + s2y * ri, 0);
            GL.Vertex3(cx + c2x * ro, cy + s2y * ro, 0);
            GL.Vertex3(cx + c1x * ri, cy + s1y * ri, 0);
            GL.Vertex3(cx + c2x * ro, cy + s2y * ro, 0);
            GL.Vertex3(cx + c1x * ro, cy + s1y * ro, 0);
        }
        GL.End();
    }

    void DrawEllipse(float cx, float cy, float rx, float ry, Color col, int segs = 24)
    {
        if (rx <= 0 || ry <= 0) return;
        GL.Begin(GL.TRIANGLES);
        GL.Color(col);
        for (int i = 0; i < segs; i++)
        {
            float a1 = i * 2f * Mathf.PI / segs;
            float a2 = (i + 1) * 2f * Mathf.PI / segs;
            GL.Vertex3(cx, cy, 0);
            GL.Vertex3(cx + Mathf.Cos(a1) * rx, cy + Mathf.Sin(a1) * ry, 0);
            GL.Vertex3(cx + Mathf.Cos(a2) * rx, cy + Mathf.Sin(a2) * ry, 0);
        }
        GL.End();
    }

    void DrawEllipseOutline(float cx, float cy, float rx, float ry, Color col, float w, int segs = 32)
    {
        Vector2 prev = new Vector2(cx + rx, cy);
        for (int i = 1; i <= segs; i++)
        {
            float ang = i * 2f * Mathf.PI / segs;
            Vector2 curr = new Vector2(cx + Mathf.Cos(ang) * rx, cy + Mathf.Sin(ang) * ry);
            DrawLine(prev.x, prev.y, curr.x, curr.y, col, w);
            prev = curr;
        }
    }

    void DrawSemiCircleTop(float cx, float cy, float r, Color col, int segs = 16)
    {
        GL.Begin(GL.TRIANGLES);
        GL.Color(col);
        for (int i = 0; i < segs; i++)
        {
            float a1 = Mathf.PI + i * Mathf.PI / segs;
            float a2 = Mathf.PI + (i + 1) * Mathf.PI / segs;
            GL.Vertex3(cx, cy, 0);
            GL.Vertex3(cx + Mathf.Cos(a1) * r, cy + Mathf.Sin(a1) * r, 0);
            GL.Vertex3(cx + Mathf.Cos(a2) * r, cy + Mathf.Sin(a2) * r, 0);
        }
        GL.End();
    }

    void DrawLine(float x1, float y1, float x2, float y2, Color c, float thickness)
    {
        // Render as a thin quad
        Vector2 d = new Vector2(x2 - x1, y2 - y1);
        float len = d.magnitude;
        if (len < 0.0001f) return;
        d /= len;
        Vector2 n = new Vector2(-d.y, d.x) * (thickness * 0.5f);
        GL.Begin(GL.TRIANGLES);
        GL.Color(c);
        GL.Vertex3(x1 - n.x, y1 - n.y, 0);
        GL.Vertex3(x2 - n.x, y2 - n.y, 0);
        GL.Vertex3(x2 + n.x, y2 + n.y, 0);
        GL.Vertex3(x1 - n.x, y1 - n.y, 0);
        GL.Vertex3(x2 + n.x, y2 + n.y, 0);
        GL.Vertex3(x1 + n.x, y1 + n.y, 0);
        GL.End();
    }

    void StrokeRect(float x, float y, float w, float h, Color c, float thick)
    {
        DrawRect(x, y, w, thick, c);
        DrawRect(x, y + h - thick, w, thick, c);
        DrawRect(x, y, thick, h, c);
        DrawRect(x + w - thick, y, thick, h, c);
    }

    void DrawVGradient(float x, float y, float w, float h, Color top, Color bottom, int bands = 16)
    {
        for (int i = 0; i < bands; i++)
        {
            float tt = i / (float)(bands - 1);
            Color c = Color.Lerp(top, bottom, tt);
            float yy = y + h * i / bands;
            float hh = h / bands + 1;
            DrawRect(x, yy, w, hh, c);
        }
    }

    // ===== Text helpers (OnGUI side) =====
    void SetText(int fontSize, Color c, TextAnchor align)
    {
        textStyle.fontSize = fontSize;
        textStyle.normal.textColor = c;
        textStyle.alignment = align;
    }
    void DrawTextCentered(string text, float y, float dx)
    {
        GUI.Label(new Rect(0 + dx, y, W, 80), text, textStyle);
    }
    void DrawTextAt(string text, float cx, float y, float boxW)
    {
        GUI.Label(new Rect(cx - boxW / 2f, y, boxW, 40), text, textStyle);
    }
    void DrawTextAtRaw(string text, float x, float y, float boxW)
    {
        GUI.Label(new Rect(x, y, boxW, 40), text, textStyle);
    }

    // ===== Audio =====
    void SetupAudio()
    {
        sfxClips["hit"]     = MakeSfx(220, 0.06f, "square", 0.4f, -100);
        sfxClips["heavy"]   = MakeSfx(120, 0.15f, "saw",    0.5f, -80);
        sfxClips["block"]   = MakeSfx(800, 0.04f, "square", 0.3f, 0);
        sfxClips["whoosh"]  = MakeNoise(0.08f, 0.3f);
        sfxClips["jump"]    = MakeSfx(300, 0.10f, "square", 0.3f, 200);
        sfxClips["fire"]    = MakeSfx(400, 0.20f, "saw",    0.4f, -200);
        sfxClips["ko"]      = MakeSfx(200, 0.50f, "saw",    0.5f, -180);
        sfxClips["select"]  = MakeSfx(660, 0.05f, "square", 0.3f, 0);
        sfxClips["confirm"] = MakeSfx(880, 0.12f, "square", 0.4f, 200);
    }

    void PlaySfx(string name)
    {
        if (!sfxClips.ContainsKey(name)) return;
        var src = audioSources[audioIndex];
        audioIndex = (audioIndex + 1) % SFX_PLAYER_COUNT;
        src.PlayOneShot(sfxClips[name]);
    }

    AudioClip MakeSfx(float freq, float duration, string shape, float volume, float slide)
    {
        int sampleRate = 22050;
        int sampleCount = Mathf.Max(1, (int)(sampleRate * duration));
        float[] data = new float[sampleCount];
        float phase = 0;
        for (int i = 0; i < sampleCount; i++)
        {
            float ti = (float)i / sampleRate;
            float f = freq + slide * ti;
            phase += f / sampleRate;
            float v;
            switch (shape)
            {
                case "square": v = Mod(phase, 1f) < 0.5f ? 1f : -1f; break;
                case "saw":    v = 2f * Mod(phase, 1f) - 1f; break;
                default:       v = Mathf.Sin(phase * 2f * Mathf.PI); break;
            }
            v *= (1f - (float)i / sampleCount) * volume;
            data[i] = Mathf.Clamp(v, -1f, 1f);
        }
        var clip = AudioClip.Create("sfx_" + shape + "_" + freq, sampleCount, 1, sampleRate, false);
        clip.SetData(data, 0);
        return clip;
    }

    AudioClip MakeNoise(float duration, float volume)
    {
        int sampleRate = 22050;
        int sampleCount = Mathf.Max(1, (int)(sampleRate * duration));
        float[] data = new float[sampleCount];
        for (int i = 0; i < sampleCount; i++)
        {
            float v = (Random.value * 2f - 1f) * (1f - (float)i / sampleCount) * volume;
            data[i] = Mathf.Clamp(v, -1f, 1f);
        }
        var clip = AudioClip.Create("sfx_noise", sampleCount, 1, sampleRate, false);
        clip.SetData(data, 0);
        return clip;
    }

    // ===== Misc helpers =====
    static Color H1(string hex)
    {
        Color c;
        if (ColorUtility.TryParseHtmlString(hex, out c)) return c;
        return Color.magenta; // visible if parse fails
    }

    static float Mod(float a, float b)
    {
        float m = a % b;
        if (m < 0) m += b;
        return m;
    }
}
