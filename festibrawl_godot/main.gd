extends Node2D
# ============================================================
# FESTIBRAWL — Holiday Mascot Brawl
# Single-file Godot 4 port. All rendering is procedural via _draw().
# ============================================================

# ---------- Display constants ----------
const W: int = 960
const H: int = 540
const GROUND: float = float(H) - 70.0
const GRAVITY: float = 0.85
const ARENA_LEFT: float = 40.0
const ARENA_RIGHT: float = float(W) - 40.0

# ---------- Roster ----------
const ROSTER: Array = [
	{"id":"santa",   "name":"SAN T. CLAWS",   "title":"YULETIDE BRAWLER", "primary":"#d42028", "accent":"#f5f5f5", "skin":"#ffd0a8",
	 "hp":135, "speed":3.4, "jump":11.0, "power":1.35, "weight":1.5, "build":"huge",
	 "special":"boulder",   "desc":"Hurls a sack of gifts."},
	{"id":"easter",  "name":"ESTHER BUNNY",   "title":"BASKET BANDIT",    "primary":"#f0a0c8", "accent":"#fff080", "skin":"#ffe8d0",
	 "hp":88,  "speed":6.6, "jump":18.0, "power":0.85, "weight":0.85, "build":"lean",
	 "special":"fireball",  "desc":"Throws explosive eggs."},
	{"id":"jack",    "name":"JACK O'LANTERN", "title":"PATCH PHANTOM",    "primary":"#ff7820", "accent":"#5a2080", "skin":"#ff7820",
	 "hp":85,  "speed":6.0, "jump":16.0, "power":1.0, "weight":0.9, "build":"lean",
	 "special":"teleport",  "desc":"Vanishes in spirit smoke."},
	{"id":"turkey",  "name":"TOM GOBBLESON",  "title":"HARVEST KING",     "primary":"#6a3818", "accent":"#e0a428", "skin":"#e8a060",
	 "hp":110, "speed":4.6, "jump":13.0, "power":1.15, "weight":1.2, "build":"broad",
	 "special":"tornado",   "desc":"Rising feather twister."},
	{"id":"cupid",   "name":"CU PID",         "title":"LOVE-STRUCK",      "primary":"#ffb0c8", "accent":"#ffd040", "skin":"#ffd8c0",
	 "hp":90,  "speed":5.2, "jump":17.0, "power":1.0, "weight":0.9, "build":"small",
	 "special":"lightbeam", "desc":"Long-range heart arrow."},
	{"id":"patty",   "name":"PATTY O'LUCKY",  "title":"GREEN MENACE",     "primary":"#2a8038", "accent":"#ffd040", "skin":"#f0c890",
	 "hp":88,  "speed":7.2, "jump":17.0, "power":0.85, "weight":0.85, "build":"small",
	 "special":"dash",      "desc":"Lucky-charm rush strike."},
	{"id":"sam",     "name":"UNCLE SAM",      "title":"STAR-SPANGLED",    "primary":"#2a4090", "accent":"#d42028", "skin":"#ffd8b0",
	 "hp":105, "speed":4.4, "jump":13.0, "power":1.1, "weight":1.15, "build":"broad",
	 "special":"iceshard",  "desc":"Fires patriot rockets."},
	{"id":"macabee", "name":"MAC A. BEE",     "title":"MENORAH KNIGHT",   "primary":"#2840a0", "accent":"#e0c860", "skin":"#d8a880",
	 "hp":95,  "speed":5.0, "jump":14.0, "power":1.0, "weight":1.0, "build":"lean",
	 "special":"poison",    "desc":"Spinning dreidel hex."},
]

const BACKGROUNDS: Array = ["workshop", "graveyard", "wonderland", "fireworks"]

# ---------- Game state ----------
var state: String = "title"
var frame: int = 0
var shake: float = 0.0
var flash: float = 0.0
var bg_choice: int = 0

# Selection
var sel: Dictionary = {}

# Fight
var fighters: Array = []
var projectiles: Array = []
var particles: Array = []
var timer_frames: int = 60 * 60
var rounds_p1: int = 0
var rounds_p2: int = 0
var round_end_counter: int = 0
var announce_text: String = ""
var announce_timer: int = 0

# Font for HUD/UI
var ui_font: Font

# ---------- Audio ----------
var sfx_streams: Dictionary = {}
var sfx_players: Array = []
var sfx_index: int = 0
const SFX_PLAYER_COUNT: int = 10


# ============================================================
# READY
# ============================================================
func _ready() -> void:
	randomize()
	ui_font = ThemeDB.fallback_font
	_setup_audio()
	# Initial selection state
	sel = _new_selection_state()


func _new_selection_state() -> Dictionary:
	return {
		"p1": 0,
		"p2": 0,
		"p1_locked": false,
		"p2_locked": false,
		"mode": "cpu",
		"mode_chosen": false,
	}


# ============================================================
# MAIN LOOP
# ============================================================
func _process(_delta: float) -> void:
	frame += 1
	match state:
		"title": _update_title()
		"select": _update_select()
		"fight": _update_fight()
		"roundover": _update_round_over()
		"matchover": _update_match_over()
	shake = max(0.0, shake - 1.0)
	flash = max(0.0, flash - 1.0)
	announce_timer = max(0, announce_timer - 1)
	queue_redraw()


func _draw() -> void:
	# Apply screen shake via transform
	var shake_offset: Vector2 = Vector2.ZERO
	if shake > 0:
		shake_offset = Vector2(randf_range(-shake, shake), randf_range(-shake, shake)) * 0.5
	draw_set_transform(shake_offset, 0.0, Vector2.ONE)

	match state:
		"title":
			_draw_title()
		"select":
			_draw_select()
		_:
			_draw_arena()
			_draw_fight_scene()

	# Hit flash overlay
	if flash > 0:
		draw_rect(Rect2(0, 0, W, H), Color(1, 1, 1, flash / 4.0))

	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

	# Vignette
	for i in range(4):
		var a: float = float(i) * 0.12
		var pad: float = float(i) * 12.0
		draw_rect(Rect2(0, 0, W, pad), Color(0, 0, 0, a))
		draw_rect(Rect2(0, H - pad, W, pad), Color(0, 0, 0, a))
		draw_rect(Rect2(0, 0, pad, H), Color(0, 0, 0, a))
		draw_rect(Rect2(W - pad, 0, pad, H), Color(0, 0, 0, a))


# ============================================================
# STATE: TITLE
# ============================================================
func _update_title() -> void:
	if Input.is_action_just_pressed("ui_confirm"):
		_play_sfx("confirm")
		state = "select"
		sel = _new_selection_state()


func _draw_title() -> void:
	# Background
	draw_rect(Rect2(0, 0, W, H), H("#0a0510"))
	# Drifting confetti
	for i in range(50):
		var x: float = fmod(float(i) * 73.0 + float(frame) * 0.5, float(W))
		var y: float = fmod(float(i * 37), float(H))
		var c: Color
		match i % 4:
			0: c = H("#ff4040")
			1: c = H("#40dd40")
			2: c = H("#ffaa40")
			_: c = H("#80c0ff")
		c.a = 0.25
		draw_rect(Rect2(x, y, 3, 3), c)

	# Title
	var t: String = "FESTIBRAWL"
	var fs: int = 56
	var w_text: float = ui_font.get_string_size(t, HORIZONTAL_ALIGNMENT_LEFT, -1, fs).x
	var tx: float = (W - w_text) / 2.0
	# Shadow stack
	for i in range(6, 0, -1):
		var col: Color = Color(0.3 + i*0.04, 0.05 + i*0.02, 0.05 + i*0.02)
		draw_string(ui_font, Vector2(tx + i, 180 + i), t, HORIZONTAL_ALIGNMENT_LEFT, -1, fs, col)
	draw_string(ui_font, Vector2(tx, 180), t, HORIZONTAL_ALIGNMENT_LEFT, -1, fs, H("#ffd040"))

	# Subtitle
	_draw_centered_text("— HOLIDAY MASCOT BRAWL —", 220, 14, H("#ff6644"))

	# Demo fighters flanking VS
	var idx_l: int = (frame / 120) % ROSTER.size()
	var idx_r: int = (idx_l + 4) % ROSTER.size()
	var bob: float = sin(frame * 0.05) * 4.0
	_draw_fighter_sprite(W/2.0 - 240, 380 + bob, ROSTER[idx_l], 1, "idle", 0, 1.0, null, 0.0, false, false)
	_draw_fighter_sprite(W/2.0 + 240, 380 - bob, ROSTER[idx_r], -1, "idle", 0, 1.0, null, 0.0, false, false)
	_draw_centered_text("VS", 380, 48, H("#ff4040"))

	# Press start (blinking)
	if (frame / 30) % 2 == 0:
		_draw_centered_text("PRESS ENTER TO START", 480, 16, H("#f5e9c4"))

	_draw_centered_text("(C) 2026  FESTIBRAWL  |  8 MASCOTS  |  BEST OF 3", 515, 9, H("#8a7a5a"))


# ============================================================
# STATE: SELECT
# ============================================================
func _update_select() -> void:
	if not sel.mode_chosen:
		if Input.is_action_just_pressed("p1_left") or Input.is_action_just_pressed("p2_left"):
			sel.mode = "cpu"; _play_sfx("select")
		if Input.is_action_just_pressed("p1_right") or Input.is_action_just_pressed("p2_right"):
			sel.mode = "2p"; _play_sfx("select")
		if Input.is_action_just_pressed("ui_confirm"):
			sel.mode_chosen = true; _play_sfx("confirm")
		if Input.is_action_just_pressed("ui_back"):
			state = "title"; _play_sfx("select")
		return

	# P1 navigates with WASD
	if not sel.p1_locked:
		if Input.is_action_just_pressed("p1_left"):  sel.p1 = (sel.p1 + ROSTER.size() - 1) % ROSTER.size(); _play_sfx("select")
		if Input.is_action_just_pressed("p1_right"): sel.p1 = (sel.p1 + 1) % ROSTER.size(); _play_sfx("select")
		if Input.is_action_just_pressed("p1_up"):    sel.p1 = (sel.p1 + ROSTER.size() - 4) % ROSTER.size(); _play_sfx("select")
		if Input.is_action_just_pressed("p1_down"):  sel.p1 = (sel.p1 + 4) % ROSTER.size(); _play_sfx("select")
		if Input.is_action_just_pressed("p1_punch") or Input.is_action_just_pressed("ui_confirm"):
			sel.p1_locked = true; _play_sfx("confirm")

	# P2 navigates with arrows (or auto-pick if vs CPU)
	if sel.p1_locked and not sel.p2_locked:
		if sel.mode == "2p":
			if Input.is_action_just_pressed("p2_left"):  sel.p2 = (sel.p2 + ROSTER.size() - 1) % ROSTER.size(); _play_sfx("select")
			if Input.is_action_just_pressed("p2_right"): sel.p2 = (sel.p2 + 1) % ROSTER.size(); _play_sfx("select")
			if Input.is_action_just_pressed("p2_up"):    sel.p2 = (sel.p2 + ROSTER.size() - 4) % ROSTER.size(); _play_sfx("select")
			if Input.is_action_just_pressed("p2_down"):  sel.p2 = (sel.p2 + 4) % ROSTER.size(); _play_sfx("select")
			if Input.is_action_just_pressed("p2_punch") or Input.is_action_just_pressed("ui_confirm"):
				sel.p2_locked = true; _play_sfx("confirm")
		else:
			# CPU picks a different character
			sel.p2 = (sel.p1 + 1 + randi() % (ROSTER.size() - 1)) % ROSTER.size()
			sel.p2_locked = true
			_play_sfx("confirm")

	if sel.p1_locked and sel.p2_locked:
		bg_choice = randi() % BACKGROUNDS.size()
		state = "fight"
		rounds_p1 = 0
		rounds_p2 = 0
		_start_round()

	if Input.is_action_just_pressed("ui_back"):
		if sel.p2_locked:
			sel.p2_locked = false
		elif sel.p1_locked:
			sel.p1_locked = false
		elif sel.mode_chosen:
			sel.mode_chosen = false
		else:
			state = "title"
		_play_sfx("select")


func _draw_select() -> void:
	draw_rect(Rect2(0, 0, W, H), H("#0a0510"))
	# Grid background
	var grid_color: Color = H("#2a1a3a")
	for i in range(0, W, 30):
		draw_line(Vector2(i, 0), Vector2(i, H), grid_color, 1.0)
	for j in range(0, H, 30):
		draw_line(Vector2(0, j), Vector2(W, j), grid_color, 1.0)

	if not sel.mode_chosen:
		_draw_centered_text("SELECT MODE", 130, 24, H("#ffd040"))
		var opts: Array = [["cpu", "VS CPU"], ["2p", "VS PLAYER 2"]]
		for i in range(opts.size()):
			var val: String = opts[i][0]
			var label: String = opts[i][1]
			var x: float = W/2.0 + (-160.0 if i == 0 else 160.0)
			var selected: bool = sel.mode == val
			var bg_col: Color = H("#ff4040") if selected else H("#3a2a4a")
			draw_rect(Rect2(x - 130, 240, 260, 90), bg_col)
			var border: Color = H("#ffd040") if selected else H("#6a4a7a")
			_stroke_rect(Rect2(x - 130, 240, 260, 90), border, 3)
			_draw_centered_text_at(label, x, 280, 20, Color.WHITE)
		_draw_centered_text("LEFT/RIGHT  ENTER to confirm", 420, 12, H("#8a7a5a"))
		return

	# Character grid
	_draw_centered_text("SELECT YOUR FIGHTER", 50, 20, H("#ffd040"))
	var cols: int = 4
	var cell_w: float = 130.0
	var cell_h: float = 130.0
	var grid_x: float = (W - cols * cell_w) / 2.0
	var grid_y: float = 80.0
	for i in range(ROSTER.size()):
		var c: Dictionary = ROSTER[i]
		var cx: float = grid_x + (i % cols) * cell_w
		var cy: float = grid_y + (i / cols) * cell_h
		# Cell background
		draw_rect(Rect2(cx + 5, cy + 5, cell_w - 10, cell_h - 10), H("#1a0a25"))
		# P1 highlight
		var is_p1: bool = (i == sel.p1)
		var is_p2: bool = (i == sel.p2)
		if is_p1 and not sel.p1_locked:
			var col: Color = H("#ff4040") if frame % 20 < 10 else H("#ffaa40")
			_stroke_rect(Rect2(cx + 5, cy + 5, cell_w - 10, cell_h - 10), col, 4)
		elif is_p1 and sel.p1_locked:
			_stroke_rect(Rect2(cx + 5, cy + 5, cell_w - 10, cell_h - 10), H("#ff4040"), 4)
		if sel.p1_locked and sel.mode == "2p" and is_p2:
			var col2: Color
			if not sel.p2_locked:
				col2 = H("#4080ff") if frame % 20 < 10 else H("#80c0ff")
			else:
				col2 = H("#4080ff")
			_stroke_rect(Rect2(cx + 8, cy + 8, cell_w - 16, cell_h - 16), col2, 4)
		# Color swatch (faint)
		var swatch: Color = H(c.primary)
		swatch.a = 0.18
		draw_rect(Rect2(cx + 12, cy + 12, cell_w - 24, cell_h - 50), swatch)
		# Mini fighter portrait
		_draw_fighter_sprite(cx + cell_w/2.0, cy + 90, c, 1, "idle", 0, 0.55, null, 0.0, false, false)
		# Name
		_draw_centered_text_at(c.name, cx + cell_w/2.0, cy + cell_h - 12, 10, Color.WHITE)

	# Side panels
	_draw_side_panel(40, 240, ROSTER[sel.p1], "P1", H("#ff4040"), sel.p1_locked)
	var p2_ready: bool = sel.p2_locked or (sel.mode == "cpu" and sel.p1_locked)
	_draw_side_panel(W - 220, 240, ROSTER[sel.p2], "P2", H("#4080ff"), p2_ready)

	# Hint
	if not sel.p1_locked:
		_draw_centered_text("P1: WASD navigate, F to lock in", H - 30, 10, H("#8a7a5a"))
	elif not sel.p2_locked and sel.mode == "2p":
		_draw_centered_text("P2: ARROWS navigate, J to lock in", H - 30, 10, H("#8a7a5a"))
	else:
		_draw_centered_text("LOADING...", H - 30, 10, H("#8a7a5a"))


func _draw_side_panel(x: float, y: float, char_def: Dictionary, label: String, color: Color, locked: bool) -> void:
	draw_rect(Rect2(x, y, 180, 180), H("#1a0a25"))
	_stroke_rect(Rect2(x, y, 180, 180), color, 3)
	draw_rect(Rect2(x, y, 180, 24), color)
	draw_string(ui_font, Vector2(x + 10, y + 17), label + (" READY" if locked else ""), HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color.WHITE)
	draw_string(ui_font, Vector2(x + 10, y + 50), char_def.name, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, H(char_def.primary))
	draw_string(ui_font, Vector2(x + 10, y + 68), char_def.title, HORIZONTAL_ALIGNMENT_LEFT, -1, 8, H("#aaaaaa"))
	# Stats
	_draw_stat_bar(x + 10, y + 90, "HP",  float(char_def.hp) / 140.0)
	_draw_stat_bar(x + 10, y + 108, "SPD", float(char_def.speed) / 8.0)
	_draw_stat_bar(x + 10, y + 126, "PWR", float(char_def.power) / 1.5)
	# Description
	var words: Array = char_def.desc.split(" ")
	var lines: Array = []
	var line: String = ""
	for w in words:
		if (line + w).length() > 24:
			lines.append(line)
			line = w + " "
		else:
			line += w + " "
	lines.append(line)
	for i in range(lines.size()):
		draw_string(ui_font, Vector2(x + 10, y + 152 + i * 10), lines[i].strip_edges(), HORIZONTAL_ALIGNMENT_LEFT, -1, 7, H("#dddddd"))


func _draw_stat_bar(x: float, y: float, label: String, frac: float) -> void:
	draw_string(ui_font, Vector2(x, y + 8), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 7, H("#dddddd"))
	var bx: float = x + 32
	var bw: float = 120
	draw_rect(Rect2(bx, y, bw, 10), Color.BLACK)
	draw_rect(Rect2(bx, y, bw * clamp(frac, 0.0, 1.0), 10), H("#ffd040"))
	_stroke_rect(Rect2(bx, y, bw, 10), H("#6a4a1a"), 1)


# ============================================================
# STATE: FIGHT
# ============================================================
func _start_round() -> void:
	fighters = [
		_make_fighter(ROSTER[sel.p1], 220, 1, false),
		_make_fighter(ROSTER[sel.p2], W - 220, -1, sel.mode == "cpu"),
	]
	fighters[0].facing = 1
	fighters[1].facing = -1
	projectiles.clear()
	particles.clear()
	timer_frames = 60 * 60
	round_end_counter = 0
	_announce("ROUND %d" % (rounds_p1 + rounds_p2 + 1), 90)
	# Schedule "FIGHT!" announcement
	get_tree().create_timer(1.5).timeout.connect(func(): _announce("FIGHT!", 60))


func _make_fighter(char_def: Dictionary, x: float, side: int, cpu: bool) -> Dictionary:
	var width: int
	var height: int
	match char_def.build:
		"huge": width = 56; height = 110
		"broad": width = 50; height = 100
		"small": width = 38; height = 86
		_: width = 44; height = 100
	return {
		"char": char_def,
		"x": x, "y": GROUND,
		"vx": 0.0, "vy": 0.0,
		"side": side,
		"facing": side,
		"width": width, "height": height,
		"hp": char_def.hp, "hp_max": char_def.hp,
		"display_hp": float(char_def.hp),
		"state": "idle",
		"state_timer": 0,
		"on_ground": true,
		"blocking": false,
		"crouching": false,
		"invuln": 0,
		"hitstun": 0,
		"active_attack": null,
		"cooldown": 0,
		"poisoned": 0,
		"cpu": cpu,
		"ai_timer": 0,
		"ai_action": "idle",
		"flash_timer": 0,
		"walk_phase": 0.0,
	}


func _announce(text: String, time: int) -> void:
	announce_text = text
	announce_timer = time


func _update_fight() -> void:
	if announce_timer > 30:
		return
	timer_frames = max(0, timer_frames - 1)

	# Auto-face each other
	if fighters[0].on_ground:
		fighters[0].facing = 1 if fighters[0].x < fighters[1].x else -1
	if fighters[1].on_ground:
		fighters[1].facing = 1 if fighters[1].x < fighters[0].x else -1

	# Inputs / AI
	for i in range(fighters.size()):
		var f: Dictionary = fighters[i]
		if f.state == "ko":
			continue
		if f.cpu:
			_ai_input(f, fighters[1 - i])
		else:
			_handle_input(f, i)

	# Physics
	for f in fighters:
		_update_fighter(f)

	# Projectiles
	for i in range(projectiles.size() - 1, -1, -1):
		var p: Dictionary = projectiles[i]
		p.x += p.vx
		p.y += p.vy
		if p.has("gravity") and p.gravity:
			p.vy += 0.4
		p.life -= 1
		# Collision with fighters
		for f in fighters:
			if f == p.owner or f.state == "ko" or f.invuln > 0 or p.hit:
				continue
			var dx: float = abs(f.x - p.x)
			var dy: float = abs((f.y - 50.0) - p.y)
			if dx < (f.width / 2.0 + p.w / 2.0) and dy < (f.height / 2.0 + p.h / 2.0):
				_apply_hit(f, p.dmg, sign(p.vx) * 5.0, p.owner.char.power)
				if p.has("poisons") and p.poisons:
					f.poisoned = 180
				p.hit = true
				p.life = 0
				var color: Color
				match p.type:
					"fire": color = H("#ffaa20")
					"ice": color = H("#a8e0ff")
					"rock": color = H("#8a6a3a")
					"beam": color = H("#ffe080")
					_: color = H("#80ff40")
				_spawn_hit_particles(p.x, p.y, color, 8)
				break
		if p.life <= 0 or p.x < 0 or p.x > W or p.y > GROUND + 20:
			projectiles.remove_at(i)

	# Melee hits
	for f in fighters:
		if f.active_attack == null or f.active_attack.hit:
			continue
		var a: Dictionary = f.active_attack
		if a.hit_on.size() == 2:
			var elapsed: int = a.frames - f.state_timer
			if elapsed >= a.hit_on[0] and elapsed <= a.hit_on[1]:
				var opp: Dictionary = _other(f)
				if opp.state != "ko" and opp.invuln <= 0:
					var dx: float = abs(f.x - opp.x)
					if dx < a.range and abs(f.y - opp.y) < 90 and sign(opp.x - f.x) == f.facing:
						_apply_hit(opp, a.dmg, f.facing * a.knockback, f.char.power)
						a.hit = true

	# Particles
	for i in range(particles.size() - 1, -1, -1):
		var p2: Dictionary = particles[i]
		p2.x += p2.vx
		p2.y += p2.vy
		p2.vy += p2.gravity
		p2.life -= 1
		if p2.life <= 0:
			particles.remove_at(i)

	# Poison DoT
	for f in fighters:
		if f.poisoned > 0 and f.state != "ko":
			f.poisoned -= 1
			if f.poisoned % 30 == 0:
				f.hp = max(0, f.hp - 2)
				_spawn_hit_particles(f.x, f.y - 50, H("#80ff40"), 4)
				if f.hp <= 0:
					f.state = "ko"
					f.state_timer = 999
					_play_sfx("ko")

	# Round-end check
	if fighters[0].state == "ko" or fighters[1].state == "ko" or timer_frames == 0:
		state = "roundover"
		round_end_counter = 150
		var winner: int = -1
		if fighters[0].state == "ko" and fighters[1].state == "ko":
			winner = -1
		elif fighters[0].state == "ko":
			winner = 1; rounds_p2 += 1
		elif fighters[1].state == "ko":
			winner = 0; rounds_p1 += 1
		elif fighters[0].hp > fighters[1].hp:
			winner = 0; rounds_p1 += 1
		elif fighters[1].hp > fighters[0].hp:
			winner = 1; rounds_p2 += 1
		if winner == 0:
			_announce("%s WINS!" % fighters[0].char.name, round_end_counter)
		elif winner == 1:
			_announce("%s WINS!" % fighters[1].char.name, round_end_counter)
		else:
			_announce("DRAW!", round_end_counter)


func _other(f: Dictionary) -> Dictionary:
	for o in fighters:
		if o != f:
			return o
	return f


func _handle_input(f: Dictionary, idx: int) -> void:
	var prefix: String = "p1_" if idx == 0 else "p2_"
	# If locked in a hit/attack, no input
	if f.state == "hit" or f.state == "punch" or f.state == "kick" or f.state == "special":
		f.blocking = false
		return
	# Block (hold down on ground)
	f.blocking = Input.is_action_pressed(prefix + "down") and f.on_ground

	if f.on_ground and not f.blocking:
		var moving: bool = false
		if Input.is_action_pressed(prefix + "left") and f.x > ARENA_LEFT:
			f.vx = -f.char.speed; moving = true
		elif Input.is_action_pressed(prefix + "right") and f.x < ARENA_RIGHT:
			f.vx = f.char.speed; moving = true
		else:
			f.vx *= 0.6
		f.crouching = Input.is_action_pressed(prefix + "down") and not Input.is_action_pressed(prefix + "left") and not Input.is_action_pressed(prefix + "right")
		if Input.is_action_just_pressed(prefix + "up"):
			f.vy = -f.char.jump
			f.on_ground = false
			_play_sfx("jump")
		if moving and f.state != "walk" and not f.crouching:
			f.state = "walk"
		elif not moving and not f.crouching and f.state == "walk":
			f.state = "idle"
		if f.crouching:
			f.state = "crouch"
		elif not moving and f.state == "crouch":
			f.state = "idle"
	elif f.blocking:
		f.vx *= 0.5
		f.state = "block"
	else:
		# Air control
		if Input.is_action_pressed(prefix + "left"):
			f.vx = max(f.vx - 0.4, -f.char.speed * 0.7)
		if Input.is_action_pressed(prefix + "right"):
			f.vx = min(f.vx + 0.4, f.char.speed * 0.7)

	if Input.is_action_just_pressed(prefix + "punch"):
		_start_attack(f, "punch")
	if Input.is_action_just_pressed(prefix + "kick"):
		_start_attack(f, "kick")
	if Input.is_action_just_pressed(prefix + "special"):
		_start_attack(f, "special")


func _ai_input(f: Dictionary, opp: Dictionary) -> void:
	f.ai_timer -= 1
	var dx: float = opp.x - f.x
	var dist: float = abs(dx)
	var dir: int = 1 if dx > 0 else -1

	if f.ai_timer <= 0:
		f.ai_timer = 20 + randi() % 40
		var r: float = randf()
		if dist > 220:
			if r < 0.35 and f.cooldown <= 0:
				f.ai_action = "special"
			else:
				f.ai_action = "approach"
		elif dist > 90:
			if r < 0.35: f.ai_action = "jump_in"
			elif r < 0.7: f.ai_action = "approach"
			else: f.ai_action = "wait"
		else:
			if r < 0.45: f.ai_action = "attack"
			elif r < 0.6: f.ai_action = "block"
			elif r < 0.75: f.ai_action = "retreat"
			else: f.ai_action = "jump_back"

	f.blocking = false
	if f.state == "hit" or f.state == "punch" or f.state == "kick" or f.state == "special":
		return

	match f.ai_action:
		"approach":
			if f.on_ground and f.x > ARENA_LEFT and f.x < ARENA_RIGHT:
				f.vx = dir * f.char.speed * 0.85
			f.state = "walk"
		"retreat":
			if f.on_ground and f.x > ARENA_LEFT + 10 and f.x < ARENA_RIGHT - 10:
				f.vx = -dir * f.char.speed * 0.7
			f.state = "walk"
		"jump_in":
			if f.on_ground:
				f.vy = -f.char.jump
				f.vx = dir * f.char.speed * 0.6
				f.on_ground = false
				_play_sfx("jump")
		"jump_back":
			if f.on_ground:
				f.vy = -f.char.jump
				f.vx = -dir * f.char.speed * 0.6
				f.on_ground = false
				_play_sfx("jump")
		"attack":
			_start_attack(f, "punch" if randf() < 0.5 else "kick")
		"special":
			_start_attack(f, "special")
		"block":
			f.blocking = true
			f.vx *= 0.5
			f.state = "block"
		"wait":
			f.vx *= 0.6
			f.state = "idle"


func _start_attack(f: Dictionary, type: String) -> void:
	if f.state == "hit" or f.state == "ko" or f.active_attack != null:
		return
	if f.cooldown > 0 and type == "special":
		return
	if not f.on_ground and type != "punch" and type != "kick":
		return
	var atk: Dictionary
	if type == "punch":
		atk = {"type": type, "frames": 18, "hit_on": [6, 9], "dmg": 6.0, "knockback": 3.0, "range": 56.0, "hit": false}
		_play_sfx("whoosh")
	elif type == "kick":
		atk = {"type": type, "frames": 26, "hit_on": [10, 16], "dmg": 11.0, "knockback": 6.0, "range": 70.0, "hit": false}
		_play_sfx("whoosh")
	else:
		atk = _start_special(f)
		if atk.is_empty():
			return
		f.cooldown = 90
	f.active_attack = atk
	f.state = type
	f.state_timer = atk.frames


func _start_special(f: Dictionary) -> Dictionary:
	var c: Dictionary = f.char
	var dir: int = f.facing
	match c.special:
		"fireball":
			_play_sfx("fire")
			var fc = func(): _spawn_projectile(f, "fire")
			get_tree().create_timer(0.28).timeout.connect(fc)
			return {"type":"special", "frames":36, "hit_on":[], "dmg":0.0, "range":0.0, "hit":false}
		"iceshard":
			_play_sfx("fire")
			var fc2 = func(): _spawn_projectile(f, "ice")
			get_tree().create_timer(0.35).timeout.connect(fc2)
			return {"type":"special", "frames":44, "hit_on":[], "dmg":0.0, "range":0.0, "hit":false}
		"boulder":
			_play_sfx("heavy")
			var fc3 = func(): _spawn_projectile(f, "rock")
			get_tree().create_timer(0.40).timeout.connect(fc3)
			return {"type":"special", "frames":50, "hit_on":[], "dmg":0.0, "range":0.0, "hit":false}
		"lightbeam":
			_play_sfx("fire")
			var fc4 = func(): _spawn_projectile(f, "beam")
			get_tree().create_timer(0.30).timeout.connect(fc4)
			return {"type":"special", "frames":40, "hit_on":[], "dmg":0.0, "range":0.0, "hit":false}
		"poison":
			_play_sfx("fire")
			var fc5 = func(): _spawn_projectile(f, "poison")
			get_tree().create_timer(0.28).timeout.connect(fc5)
			return {"type":"special", "frames":36, "hit_on":[], "dmg":0.0, "range":0.0, "hit":false}
		"dash":
			_play_sfx("whoosh")
			f.invuln = 18
			f.vx = dir * 18.0
			return {"type":"special", "frames":22, "hit_on":[2,18], "dmg":14.0, "knockback":8.0, "range":60.0, "hit":false}
		"tornado":
			_play_sfx("whoosh")
			f.vy = -16.0
			f.on_ground = false
			f.vx = dir * 4.0
			return {"type":"special", "frames":30, "hit_on":[2,28], "dmg":10.0, "knockback":6.0, "range":70.0, "hit":false}
		"teleport":
			_play_sfx("whoosh")
			var opp: Dictionary = _other(f)
			f.x = opp.x - opp.facing * 70.0
			f.facing = -opp.facing
			f.invuln = 20
			for i in range(12):
				particles.append(_make_particle(f.x, f.y - 50, H("#a040ff")))
			return {"type":"special", "frames":28, "hit_on":[18,24], "dmg":12.0, "knockback":6.0, "range":60.0, "hit":false}
	return {}


func _spawn_projectile(f: Dictionary, type: String) -> void:
	if not fighters.has(f) or f.state == "ko":
		return
	var dir: int = f.facing
	var px: float = f.x + dir * 40.0
	var py: float = f.y - 60.0
	var p: Dictionary = {
		"x": px, "y": py, "vx": dir * 8.0, "vy": 0.0,
		"type": type, "owner": f, "owner_id": f.char.id,
		"life": 100, "dmg": 12.0, "w": 24.0, "h": 18.0,
		"hit": false, "gravity": false, "poisons": false,
	}
	match type:
		"rock":
			p.vx = dir * 6.0; p.vy = -4.0; p.dmg = 16.0; p.w = 30.0; p.h = 28.0
			p.gravity = true; p.life = 90
		"beam":
			p.vx = dir * 14.0; p.dmg = 10.0; p.w = 50.0; p.h = 12.0; p.life = 60
		"poison":
			p.vx = dir * 5.0; p.dmg = 8.0; p.life = 140; p.poisons = true
		"ice":
			p.dmg = 14.0; p.w = 26.0; p.h = 14.0
	projectiles.append(p)


func _make_particle(x: float, y: float, color: Color) -> Dictionary:
	return {
		"x": x, "y": y,
		"vx": (randf() - 0.5) * 6.0,
		"vy": -randf() * 5.0 - 1.0,
		"life": 30 + randi() % 20,
		"color": color,
		"size": 2.0 + randf() * 3.0,
		"gravity": 0.2,
	}


func _spawn_hit_particles(x: float, y: float, color: Color, n: int) -> void:
	for i in range(n):
		particles.append(_make_particle(x, y, color))


func _update_fighter(f: Dictionary) -> void:
	if f.state == "ko":
		f.vy += GRAVITY
		f.y += f.vy
		f.x += f.vx * 0.5
		if f.y >= GROUND:
			f.y = GROUND
			f.vy = 0
			f.vx *= 0.7
		return
	f.x += f.vx
	f.y += f.vy
	if not f.on_ground:
		f.vy += GRAVITY
	f.x = clamp(f.x, ARENA_LEFT, ARENA_RIGHT)
	if f.y >= GROUND:
		f.y = GROUND
		f.vy = 0
		if not f.on_ground:
			f.on_ground = true
			if f.state == "jump" or f.state == "fall":
				f.state = "idle"
	else:
		f.on_ground = false
		if f.state != "punch" and f.state != "kick" and f.state != "special" and f.state != "hit":
			f.state = "jump" if f.vy < 0 else "fall"
	if f.state == "walk":
		f.walk_phase += 0.25
	if f.cooldown > 0: f.cooldown -= 1
	if f.invuln > 0:   f.invuln -= 1
	if f.flash_timer > 0: f.flash_timer -= 1
	if f.hitstun > 0:  f.hitstun -= 1
	if f.active_attack != null:
		f.state_timer -= 1
		if f.state_timer <= 0:
			f.active_attack = null
			f.state = "idle" if f.on_ground else "fall"
	elif f.state == "hit":
		f.state_timer -= 1
		if f.state_timer <= 0:
			f.state = "idle" if f.on_ground else "fall"
	# Push apart if overlapping
	var opp: Dictionary = _other(f)
	if f.on_ground and opp.on_ground:
		var dx: float = f.x - opp.x
		var min_dist: float = (f.width + opp.width) / 2.0 - 4.0
		if abs(dx) < min_dist:
			var push: float = (min_dist - abs(dx)) / 2.0
			f.x += sign(dx) * push * 0.5


func _apply_hit(f: Dictionary, dmg: float, kb: float, power_mul: float) -> void:
	if f.invuln > 0 or f.state == "ko":
		return
	if f.blocking and sign(kb) != f.facing:
		f.hp = max(0, f.hp - round(dmg * 0.15))
		f.vx += kb * 0.3
		_play_sfx("block")
		_spawn_hit_particles(f.x, f.y - 60, Color.WHITE, 4)
		return
	var final_dmg: int = int(round(dmg * power_mul))
	f.hp = max(0, f.hp - final_dmg)
	f.vx += kb
	f.vy = -4
	f.on_ground = false
	f.state = "hit"
	f.state_timer = 18
	f.flash_timer = 8
	f.invuln = 14
	f.active_attack = null
	var pcol: Color = H("#ff4040") if final_dmg > 10 else H("#ffaa40")
	_spawn_hit_particles(f.x, f.y - 60, pcol, 12 if final_dmg > 10 else 6)
	shake = min(20.0, shake + (12.0 if final_dmg > 10 else 5.0))
	flash = 4 if final_dmg > 10 else 2
	_play_sfx("heavy" if final_dmg > 10 else "hit")
	if f.hp <= 0:
		f.state = "ko"
		f.state_timer = 999
		f.vx = -f.facing * 8.0
		f.vy = -10.0
		f.on_ground = false
		_play_sfx("ko")
		shake = 30


func _update_round_over() -> void:
	# Continue physics so KOd fighter falls
	for f in fighters:
		_update_fighter(f)
	for i in range(particles.size() - 1, -1, -1):
		var p: Dictionary = particles[i]
		p.x += p.vx
		p.y += p.vy
		p.vy += 0.2
		p.life -= 1
		if p.life <= 0:
			particles.remove_at(i)
	round_end_counter -= 1
	if round_end_counter <= 0:
		if rounds_p1 >= 2 or rounds_p2 >= 2:
			state = "matchover"
			_announce("PLAYER 1 VICTORY" if rounds_p1 > rounds_p2 else "PLAYER 2 VICTORY", 9999)
		else:
			state = "fight"
			_start_round()


func _update_match_over() -> void:
	if Input.is_action_just_pressed("ui_confirm"):
		_play_sfx("confirm")
		state = "select"
		var prev_mode: String = sel.mode
		sel = _new_selection_state()
		sel.mode = prev_mode
		sel.mode_chosen = true
	if Input.is_action_just_pressed("ui_back"):
		_play_sfx("select")
		state = "title"


# ============================================================
# DRAW: FIGHT SCENE
# ============================================================
func _draw_fight_scene() -> void:
	# Particles behind
	for p in particles:
		var c: Color = p.color
		c.a = clamp(p.life / 30.0, 0.0, 1.0)
		draw_rect(Rect2(p.x - p.size / 2.0, p.y - p.size / 2.0, p.size, p.size), c)
	# Shadows
	for f in fighters:
		var sw: float = f.width * (1.0 if f.on_ground else 0.6)
		_draw_ellipse(Vector2(f.x, GROUND + 5), sw / 2.0, 6.0, Color(0, 0, 0, 0.4))
	# Fighters
	for f in fighters:
		_draw_fighter(f)
	# Projectiles
	for p in projectiles:
		_draw_projectile(p)
	# HUD
	_draw_hud()
	# Announcement
	if announce_timer > 0:
		var alpha: float = clamp(announce_timer / 30.0, 0.0, 1.0)
		var fs: int = 56
		var w_text: float = ui_font.get_string_size(announce_text, HORIZONTAL_ALIGNMENT_LEFT, -1, fs).x
		var ax: float = (W - w_text) / 2.0
		# Shadow
		for i in range(5, 0, -1):
			draw_string(ui_font, Vector2(ax + i, H/2.0 + i), announce_text, HORIZONTAL_ALIGNMENT_LEFT, -1, fs, Color(0, 0, 0, 0.5 * alpha))
		draw_string(ui_font, Vector2(ax, H/2.0), announce_text, HORIZONTAL_ALIGNMENT_LEFT, -1, fs, Color(1, 0.82, 0.25, alpha))
	# Match over overlay
	if state == "matchover":
		draw_rect(Rect2(0, 0, W, H), Color(0, 0, 0, 0.7))
		_draw_centered_text(announce_text, H/2 - 20, 36, H("#ffd040"))
		if (frame / 30) % 2 == 0:
			_draw_centered_text("PRESS ENTER FOR REMATCH  |  ESC TO TITLE", H/2 + 40, 14, Color.WHITE)


func _draw_fighter(f: Dictionary) -> void:
	var flashing: bool = f.flash_timer > 0 and (frame % 4 < 2)
	_draw_fighter_sprite(f.x, f.y, f.char, f.facing, f.state, f.state_timer, 1.0,
		f.active_attack, f.walk_phase, flashing, f.poisoned > 0)


func _draw_projectile(p: Dictionary) -> void:
	var dir: int = sign(p.vx) if p.vx != 0 else 1
	var oid: String = p.owner_id
	if oid == "easter" and (p.type == "fire" or p.type == "fireball"):
		# Easter egg
		var wob: float = sin(frame * 0.3) * 1.5
		_draw_ellipse(Vector2(p.x, p.y + wob), 12, 16, H("#fff080"))
		draw_rect(Rect2(p.x - 11, p.y - 5 + wob, 22, 3), H("#80c0e8"))
		draw_rect(Rect2(p.x - 10, p.y + 4 + wob, 20, 2), H("#80c0e8"))
		draw_rect(Rect2(p.x - 8, p.y - 10 + wob, 16, 2), H("#ff80a0"))
		draw_rect(Rect2(p.x - 4, p.y - 12 + wob, 3, 2), Color.WHITE)
		var sparkle: Color = H("#ffe0a0")
		sparkle.a = 0.6
		for i in range(3):
			draw_rect(Rect2(p.x - dir*(8 + i*5), p.y + sin(frame*0.4 + i)*3, 2, 2), sparkle)
	elif oid == "santa" and p.type == "rock":
		# Wrapped present
		draw_rect(Rect2(p.x - 16, p.y - 14, 32, 28), H("#a01820"))
		draw_rect(Rect2(p.x - 14, p.y - 12, 28, 24), H("#cc2030"))
		draw_rect(Rect2(p.x - 16, p.y - 3, 32, 5), H("#ffd040"))
		draw_rect(Rect2(p.x - 3, p.y - 14, 5, 28), H("#ffd040"))
		draw_circle(Vector2(p.x - 5, p.y - 16), 4, H("#ffd040"))
		draw_circle(Vector2(p.x + 5, p.y - 16), 4, H("#ffd040"))
		draw_rect(Rect2(p.x - 1, p.y - 18, 3, 4), H("#aa8020"))
	elif oid == "cupid" and p.type == "beam":
		# Heart-tipped love arrow
		draw_rect(Rect2(p.x - p.w/2.0, p.y - 1, p.w, 3), H("#a06030"))
		# Fletching
		var ft: PackedVector2Array = PackedVector2Array([
			Vector2(p.x - dir*p.w/2.0, p.y - 5),
			Vector2(p.x - dir*(p.w/2.0 + 8), p.y),
			Vector2(p.x - dir*p.w/2.0, p.y + 5)
		])
		draw_colored_polygon(ft, H("#ffb0c8"))
		# Heart arrowhead
		var hx: float = p.x + dir * p.w/2.0
		draw_circle(Vector2(hx - dir*3, p.y - 3), 4, H("#ff4060"))
		draw_circle(Vector2(hx + dir*3, p.y - 3), 4, H("#ff4060"))
		var heart: PackedVector2Array = PackedVector2Array([
			Vector2(hx - dir*6, p.y - 1),
			Vector2(hx + dir*8, p.y + 4),
			Vector2(hx - dir*1, p.y + 6)
		])
		draw_colored_polygon(heart, H("#ff4060"))
		# Sparkles
		var sp: Color = H("#ffd0e0")
		sp.a = 0.7
		for i in range(4):
			var tx: float = p.x - dir*(p.w/2.0 + 6 + i*5)
			draw_rect(Rect2(tx, p.y + sin(frame*0.5 + i)*4 - 1, 2, 2), sp)
	elif oid == "sam" and p.type == "ice":
		# Patriot rocket
		draw_rect(Rect2(p.x - p.w/2.0, p.y - p.h/2.0, p.w, p.h), Color.WHITE)
		draw_rect(Rect2(p.x - p.w/2.0, p.y - p.h/2.0 + p.h/3.0, p.w, p.h/3.0), H("#d42028"))
		var nose: PackedVector2Array = PackedVector2Array([
			Vector2(p.x + dir*p.w/2.0, p.y - p.h/2.0),
			Vector2(p.x + dir*(p.w/2.0 + 8), p.y),
			Vector2(p.x + dir*p.w/2.0, p.y + p.h/2.0)
		])
		draw_colored_polygon(nose, H("#2a4090"))
		draw_rect(Rect2(p.x - 3, p.y - 2, 5, 5), H("#ffd040"))
		var fl: float = sin(frame*0.5) * 2
		var flame: PackedVector2Array = PackedVector2Array([
			Vector2(p.x - dir*p.w/2.0, p.y - p.h/2.0),
			Vector2(p.x - dir*(p.w/2.0 + 12 + fl), p.y),
			Vector2(p.x - dir*p.w/2.0, p.y + p.h/2.0)
		])
		draw_colored_polygon(flame, H("#ffaa20"))
		var inner: PackedVector2Array = PackedVector2Array([
			Vector2(p.x - dir*p.w/2.0, p.y - p.h/4.0),
			Vector2(p.x - dir*(p.w/2.0 + 6 + fl), p.y),
			Vector2(p.x - dir*p.w/2.0, p.y + p.h/4.0)
		])
		draw_colored_polygon(inner, H("#ffe080"))
	elif oid == "macabee" and p.type == "poison":
		# Spinning dreidel hex cloud
		var spin: float = frame * 0.3
		for i in range(5):
			var ox: float = sin(spin + i) * 5
			var oy: float = cos(spin + i) * 5
			var c: Color = H("#2840a0") if i % 2 == 1 else H("#80a0ff")
			c.a = 0.7
			draw_circle(Vector2(p.x + ox, p.y + oy), 8 + i, c)
		# Mini dreidel
		draw_rect(Rect2(p.x - 5, p.y - 5, 10, 10), H("#2840a0"))
		draw_rect(Rect2(p.x - 1, p.y - 8, 2, 3), H("#e0c860"))
		var bottom: PackedVector2Array = PackedVector2Array([
			Vector2(p.x - 5, p.y + 5),
			Vector2(p.x, p.y + 10),
			Vector2(p.x + 5, p.y + 5)
		])
		draw_colored_polygon(bottom, H("#2840a0"))
		draw_rect(Rect2(p.x - 2, p.y - 2, 4, 4), H("#e0c860"))
	# Fallback themed visuals
	elif p.type == "fire":
		for i in range(5):
			var col: Color
			if i == 0: col = Color.WHITE
			elif i < 3: col = H("#ffd040")
			else: col = H("#ff4020")
			draw_rect(Rect2(p.x - p.w/2.0 + i*2, p.y - p.h/2.0 + i, p.w - i*4, p.h - i*2), col)
	elif p.type == "ice":
		draw_rect(Rect2(p.x - p.w/2.0, p.y - p.h/2.0, p.w, p.h), Color.WHITE)
		draw_rect(Rect2(p.x - p.w/2.0 + 2, p.y - p.h/2.0 + 2, p.w - 4, p.h - 4), H("#5fc8ff"))
	elif p.type == "rock":
		draw_rect(Rect2(p.x - p.w/2.0, p.y - p.h/2.0, p.w, p.h), H("#3a2a1a"))
		draw_rect(Rect2(p.x - p.w/2.0 + 3, p.y - p.h/2.0 + 3, p.w - 6, p.h - 6), H("#8a6a3a"))
	elif p.type == "beam":
		var c1: Color = Color.WHITE; c1.a = 0.9
		draw_rect(Rect2(p.x - p.w/2.0, p.y - p.h/2.0, p.w, p.h), c1)
		var c2: Color = H("#ffd040"); c2.a = 0.6
		draw_rect(Rect2(p.x - p.w/2.0 - 10, p.y - p.h, p.w + 20, p.h * 2), c2)
	elif p.type == "poison":
		for i in range(5):
			var ox: float = sin(frame*0.1 + i)*4
			var oy: float = cos(frame*0.1 + i)*4
			var c: Color = H("#3a8a2a") if i % 2 == 1 else H("#80c040")
			c.a = 0.7
			draw_circle(Vector2(p.x + ox, p.y + oy), 8 + i, c)


# ============================================================
# DRAW: HUD
# ============================================================
func _draw_hud() -> void:
	_draw_health_bar(20, 20, fighters[0], "left")
	_draw_health_bar(W - 320, 20, fighters[1], "right")
	# Names
	draw_string(ui_font, Vector2(20, 80), fighters[0].char.name, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color.WHITE)
	var name2: String = fighters[1].char.name
	var w2: float = ui_font.get_string_size(name2, HORIZONTAL_ALIGNMENT_LEFT, -1, 14).x
	draw_string(ui_font, Vector2(W - 20 - w2, 80), name2, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color.WHITE)
	# Round indicators
	for i in range(2):
		var c1: Color = H("#ffd040") if i < rounds_p1 else H("#3a2a1a")
		draw_rect(Rect2(160 + i*16, 70, 12, 12), c1)
		_stroke_rect(Rect2(160 + i*16, 70, 12, 12), H("#6a4a1a"), 1)
		var c2: Color = H("#ffd040") if i < rounds_p2 else H("#3a2a1a")
		draw_rect(Rect2(W - 160 - i*16 - 12, 70, 12, 12), c2)
		_stroke_rect(Rect2(W - 160 - i*16 - 12, 70, 12, 12), H("#6a4a1a"), 1)
	# Timer
	var seconds: int = int(ceil(timer_frames / 60.0))
	draw_rect(Rect2(W/2 - 50, 14, 100, 50), H("#1a0a25"))
	_stroke_rect(Rect2(W/2 - 50, 14, 100, 50), H("#c9a44a"), 2)
	var tcol: Color
	if seconds <= 10:
		tcol = H("#ff4040") if frame % 20 < 10 else H("#ffaa40")
	else:
		tcol = H("#ffd040")
	var tstr: String = "%02d" % seconds
	_draw_centered_text_at(tstr, W/2, 50, 28, tcol)
	# Cooldown indicators
	_draw_cd_indicator(20, 90, fighters[0])
	_draw_cd_indicator(W - 60, 90, fighters[1])


func _draw_health_bar(x: float, y: float, fighter: Dictionary, side: String) -> void:
	var w: float = 300
	var h: float = 28
	# Frame
	draw_rect(Rect2(x - 2, y - 2, w + 4, h + 4), H("#1a0a25"))
	_stroke_rect(Rect2(x - 2, y - 2, w + 4, h + 4), H("#c9a44a"), 2)
	# Background
	draw_rect(Rect2(x, y, w, h), H("#3a1a1a"))
	# Damage trail
	fighter.display_hp = max(float(fighter.hp), fighter.display_hp - 1.2)
	var frac: float = float(fighter.hp) / float(fighter.hp_max)
	var trail_frac: float = fighter.display_hp / float(fighter.hp_max)
	if side == "left":
		draw_rect(Rect2(x, y, w * trail_frac, h), H("#ffaa40"))
	else:
		draw_rect(Rect2(x + w * (1 - trail_frac), y, w * trail_frac, h), H("#ffaa40"))
	# Live HP
	var bar_color: Color
	if frac > 0.6: bar_color = H("#40dd40")
	elif frac > 0.3: bar_color = H("#ffdd40")
	else: bar_color = H("#ff4040") if frame % 20 < 10 else H("#aa2020")
	if side == "left":
		draw_rect(Rect2(x, y, w * frac, h), bar_color)
	else:
		draw_rect(Rect2(x + w * (1 - frac), y, w * frac, h), bar_color)
	# Tick marks
	for i in range(1, 10):
		draw_rect(Rect2(x + (w/10.0)*i, y, 1, h), Color(0, 0, 0, 0.4))
	# HP number
	var label: String = "HP %d/%d" % [fighter.hp, fighter.hp_max]
	if side == "left":
		draw_string(ui_font, Vector2(x + 6, y + 18), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color.WHITE)
	else:
		var lw: float = ui_font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, 10).x
		draw_string(ui_font, Vector2(x + w - 6 - lw, y + 18), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color.WHITE)


func _draw_cd_indicator(x: float, y: float, fighter: Dictionary) -> void:
	var col: Color = H("#666666") if fighter.cooldown > 0 else H("#ffd040")
	draw_string(ui_font, Vector2(x, y + 8), "SPECIAL", HORIZONTAL_ALIGNMENT_LEFT, -1, 8, col)
	var w: float = 40
	draw_rect(Rect2(x, y + 12, w, 6), H("#1a0a25"))
	var bar: Color = H("#666666") if fighter.cooldown > 0 else H("#40dd40")
	var frac: float = 1.0 - fighter.cooldown / 90.0
	draw_rect(Rect2(x, y + 12, w * frac, 6), bar)


# ============================================================
# DRAW: ARENA BACKGROUNDS
# ============================================================
func _draw_arena() -> void:
	var bg: String = BACKGROUNDS[bg_choice]
	match bg:
		"workshop": _draw_workshop_bg()
		"graveyard": _draw_graveyard_bg()
		"wonderland": _draw_wonderland_bg()
		"fireworks": _draw_fireworks_bg()


func _draw_workshop_bg() -> void:
	# Sky -> floor banded gradient
	_draw_v_gradient(Rect2(0, 0, W, GROUND), H("#5a2a1a"), H("#3a1a0a"))
	# Wood planks
	for i in range(0, W, 70):
		draw_rect(Rect2(i, 0, 4, GROUND), H("#3a1a0a"))
		draw_rect(Rect2(i + 4, 0, 1, GROUND), H("#4a2010"))
	# Christmas tree
	var tree: PackedVector2Array = PackedVector2Array([
		Vector2(W/2 - 80, GROUND),
		Vector2(W/2 - 40, GROUND - 100),
		Vector2(W/2 - 60, GROUND - 100),
		Vector2(W/2 - 30, GROUND - 160),
		Vector2(W/2 - 50, GROUND - 160),
		Vector2(W/2 - 20, GROUND - 220),
		Vector2(W/2 + 20, GROUND - 220),
		Vector2(W/2 + 50, GROUND - 160),
		Vector2(W/2 + 30, GROUND - 160),
		Vector2(W/2 + 60, GROUND - 100),
		Vector2(W/2 + 40, GROUND - 100),
		Vector2(W/2 + 80, GROUND)
	])
	draw_colored_polygon(tree, H("#1a4a2a"))
	# Star
	var star: PackedVector2Array = PackedVector2Array([
		Vector2(W/2, GROUND - 240), Vector2(W/2 + 4, GROUND - 224),
		Vector2(W/2 + 14, GROUND - 222), Vector2(W/2 + 6, GROUND - 214),
		Vector2(W/2 + 8, GROUND - 204), Vector2(W/2, GROUND - 210),
		Vector2(W/2 - 8, GROUND - 204), Vector2(W/2 - 6, GROUND - 214),
		Vector2(W/2 - 14, GROUND - 222), Vector2(W/2 - 4, GROUND - 224)
	])
	draw_colored_polygon(star, H("#ffe040"))
	# Ornaments
	var ornaments: Array = [[-30,-200,"#ff4040"],[20,-180,"#80c0ff"],[-10,-150,"#ffd040"],[30,-130,"#ff80c0"],[-40,-110,"#40dd40"],[10,-90,"#ff4040"],[-25,-60,"#ffd040"]]
	for i in range(ornaments.size()):
		var o = ornaments[i]
		draw_circle(Vector2(W/2 + o[0], GROUND + o[1]), 4, H(o[2]))
		if (frame + i*15) % 100 < 30:
			draw_rect(Rect2(W/2 + o[0] - 1, GROUND + o[1] - 1, 2, 2), Color.WHITE)
	# Stack of presents (left)
	for i in range(3):
		var px: float = 80 + i * 12
		var py: float = GROUND - (i + 1) * 22
		var pc: Array = ["#cc2030", "#2a8038", "#2a4090"]
		draw_rect(Rect2(px, py, 30 - i*3, 20), H(pc[i]))
		draw_rect(Rect2(px + 12 - i, py, 4, 20), H("#ffd040"))
		draw_rect(Rect2(px, py + 8, 30 - i*3, 4), H("#ffd040"))
	# Stack of presents (right)
	for i in range(2):
		var px2: float = W - 110 - i * 8
		var py2: float = GROUND - (i + 1) * 24
		var pc2: Array = ["#2a4090", "#cc2030"]
		draw_rect(Rect2(px2, py2, 32 - i*4, 22), H(pc2[i]))
		draw_rect(Rect2(px2 + 14 - i, py2, 4, 22), H("#ffd040"))
		draw_rect(Rect2(px2, py2 + 9, 32 - i*4, 4), H("#ffd040"))
	# Light strand
	for i in range(30, W, 50):
		var ly: float = 60 + sin(i * 0.04) * 12
		var col: Color
		match (i / 50) % 4:
			0: col = H("#ff4040")
			1: col = H("#40dd40")
			2: col = H("#ffd040")
			_: col = H("#80c0ff")
		draw_circle(Vector2(i, ly + 5), 4, col)
		if (frame + i) % 80 < 25:
			draw_rect(Rect2(i - 1, ly + 4, 2, 2), Color.WHITE)
	# Wreath
	_stroke_circle(Vector2(W/2, 110), 22, H("#1a4a2a"), 8)
	draw_circle(Vector2(W/2 - 8, 122), 3, H("#cc2030"))
	draw_circle(Vector2(W/2 + 8, 122), 3, H("#cc2030"))
	# Floor
	draw_rect(Rect2(0, GROUND, W, H - GROUND), H("#3a1a0a"))
	for i in range(0, W, 50):
		draw_rect(Rect2(i, GROUND, 2, H - GROUND), H("#2a0a05"))


func _draw_graveyard_bg() -> void:
	_draw_v_gradient(Rect2(0, 0, W, GROUND), H("#1a0530"), H("#5a2080"))
	# Moon glow
	var glow: Color = H("#ffe8a0"); glow.a = 0.25
	draw_circle(Vector2(W - 160, 110), 80, glow)
	draw_circle(Vector2(W - 160, 110), 50, H("#fff5d0"))
	# Craters
	draw_rect(Rect2(W - 180, 100, 8, 8), H("#d8c890"))
	draw_rect(Rect2(W - 145, 130, 6, 6), H("#d8c890"))
	draw_rect(Rect2(W - 135, 95, 4, 4), H("#d8c890"))
	# Stars
	for i in range(25):
		var sx: float = (i * 79) % W
		var sy: float = (i * 43) % 200
		var c: Color = Color.WHITE
		c.a = 0.4 + sin(frame*0.06 + i) * 0.4
		draw_rect(Rect2(sx, sy, 2, 2), c)
	# Bats
	for i in range(4):
		var bx: float = fmod(i * 200.0 + frame * 1.5, W + 60.0) - 30
		var by: float = 80 + sin(frame*0.05 + i*2) * 25 + i*30
		var wing: float = sin(frame*0.4 + i) * 5
		draw_rect(Rect2(bx, by, 4, 3), H("#1a0530"))
		var w1: PackedVector2Array = PackedVector2Array([
			Vector2(bx, by + 1), Vector2(bx - 9, by - wing), Vector2(bx - 4, by + 2)
		])
		draw_colored_polygon(w1, H("#1a0530"))
		var w2: PackedVector2Array = PackedVector2Array([
			Vector2(bx + 4, by + 1), Vector2(bx + 13, by - wing), Vector2(bx + 8, by + 2)
		])
		draw_colored_polygon(w2, H("#1a0530"))
	# Tombstones
	for i in range(6):
		var sx: float = 90 + i * 130 + (i % 2) * 30
		var sy: float = GROUND - 32
		draw_rect(Rect2(sx, sy, 32, 32), H("#5a4a70"))
		if i % 2 == 0:
			# Round top - approximate with semicircle polygon
			_draw_semi_circle_top(Vector2(sx + 16, sy), 16, H("#5a4a70"))
		else:
			# Cross top
			draw_rect(Rect2(sx + 14, sy - 12, 4, 14), H("#5a4a70"))
			draw_rect(Rect2(sx + 8, sy - 6, 16, 4), H("#5a4a70"))
	# Foggy ground
	draw_rect(Rect2(0, GROUND, W, H - GROUND), H("#1a0530"))
	for i in range(12):
		var fx: float = fmod(i * 110.0 + frame * 0.6, W + 80.0) - 40
		var fc: Color = H("#3a1850"); fc.a = 0.5
		_draw_ellipse(Vector2(fx, GROUND + 12), 50, 7, fc)
	# Pumpkins on ground
	for px in [220, 470, 760]:
		_draw_ellipse(Vector2(px, GROUND + 18), 14, 11, H("#ff7820"))
		draw_rect(Rect2(px - 1, GROUND + 4, 2, 4), H("#3a6020"))
		draw_rect(Rect2(px - 6, GROUND + 14, 3, 2), H("#ffe040"))
		draw_rect(Rect2(px + 3, GROUND + 14, 3, 2), H("#ffe040"))
		draw_rect(Rect2(px - 4, GROUND + 19, 8, 2), H("#ffe040"))


func _draw_wonderland_bg() -> void:
	_draw_v_gradient(Rect2(0, 0, W, GROUND), H("#1a3a5a"), H("#5a8aaa"))
	# Snow
	for i in range(80):
		var x: float = fmod(i * 53.0 + frame * 0.4, float(W))
		var y: float = fmod(i * 31.0 + frame * 0.8, GROUND)
		var c: Color = H("#e0f0ff"); c.a = 0.7
		draw_rect(Rect2(x, y, 2, 2), c)
	# Mountains
	var mt: PackedVector2Array = PackedVector2Array([
		Vector2(0, GROUND), Vector2(150, 200), Vector2(350, 160),
		Vector2(550, 220), Vector2(750, 180), Vector2(W, 240),
		Vector2(W, GROUND)
	])
	draw_colored_polygon(mt, H("#c8d8e8"))
	# Snow caps
	var c1: PackedVector2Array = PackedVector2Array([Vector2(120, 220), Vector2(150, 200), Vector2(180, 220)])
	var c2: PackedVector2Array = PackedVector2Array([Vector2(320, 180), Vector2(350, 160), Vector2(380, 180)])
	draw_colored_polygon(c1, Color.WHITE)
	draw_colored_polygon(c2, Color.WHITE)
	# Pine trees
	for tx in [400, 480, 600, 700]:
		var tree: PackedVector2Array = PackedVector2Array([
			Vector2(tx - 14, GROUND - 5), Vector2(tx, GROUND - 70), Vector2(tx + 14, GROUND - 5)
		])
		draw_colored_polygon(tree, H("#1a4a2a"))
		var snow: PackedVector2Array = PackedVector2Array([
			Vector2(tx - 14, GROUND - 5), Vector2(tx - 12, GROUND - 8),
			Vector2(tx, GROUND - 60), Vector2(tx + 12, GROUND - 8),
			Vector2(tx + 14, GROUND - 5)
		])
		var sc: Color = Color.WHITE; sc.a = 0.6
		draw_colored_polygon(snow, sc)
	# Snowman
	var smx: int = 150
	draw_circle(Vector2(smx, GROUND - 8), 16, Color.WHITE)
	draw_circle(Vector2(smx, GROUND - 30), 12, Color.WHITE)
	draw_circle(Vector2(smx, GROUND - 48), 9, Color.WHITE)
	# Carrot
	var carrot: PackedVector2Array = PackedVector2Array([
		Vector2(smx, GROUND - 48), Vector2(smx + 9, GROUND - 47), Vector2(smx, GROUND - 46)
	])
	draw_colored_polygon(carrot, H("#ff8020"))
	# Coal
	draw_rect(Rect2(smx - 4, GROUND - 51, 2, 2), H("#1a0a05"))
	draw_rect(Rect2(smx + 2, GROUND - 51, 2, 2), H("#1a0a05"))
	draw_rect(Rect2(smx - 1, GROUND - 34, 2, 2), H("#1a0a05"))
	draw_rect(Rect2(smx - 1, GROUND - 30, 2, 2), H("#1a0a05"))
	draw_rect(Rect2(smx - 1, GROUND - 26, 2, 2), H("#1a0a05"))
	# Top hat
	draw_rect(Rect2(smx - 9, GROUND - 56, 18, 2), H("#1a0a05"))
	draw_rect(Rect2(smx - 6, GROUND - 66, 12, 10), H("#1a0a05"))
	# Stick arms
	draw_line(Vector2(smx - 12, GROUND - 28), Vector2(smx - 22, GROUND - 38), H("#5a3010"), 2)
	draw_line(Vector2(smx - 22, GROUND - 38), Vector2(smx - 26, GROUND - 32), H("#5a3010"), 2)
	draw_line(Vector2(smx + 12, GROUND - 28), Vector2(smx + 22, GROUND - 38), H("#5a3010"), 2)
	draw_line(Vector2(smx + 22, GROUND - 38), Vector2(smx + 26, GROUND - 32), H("#5a3010"), 2)
	# Light strand
	for i in range(30, W, 55):
		var ly: float = 60 + sin(i * 0.05) * 12
		var ccol: Color
		match (i / 55) % 5:
			0: ccol = H("#ff4040")
			1: ccol = H("#40dd40")
			2: ccol = H("#ffd040")
			3: ccol = H("#80c0ff")
			_: ccol = H("#ff80c0")
		draw_circle(Vector2(i, ly + 5), 4, ccol)
	# Floor
	draw_rect(Rect2(0, GROUND, W, H - GROUND), H("#a8d0e8"))
	for i in range(0, W, 40):
		draw_rect(Rect2(i, GROUND + 20, 20, 2), H("#80b0d0"))


func _draw_fireworks_bg() -> void:
	_draw_v_gradient(Rect2(0, 0, W, GROUND), H("#0a0a30"), H("#5a3080"))
	# Stars
	for i in range(30):
		var sx: float = (i * 73) % W
		var sy: float = (i * 41) % 150
		var c: Color = Color.WHITE
		c.a = 0.5 + sin(frame*0.05 + i) * 0.3
		draw_rect(Rect2(sx, sy, 2, 2), c)
	# Stars-and-stripes bunting at top
	for i in range(0, W, 50):
		var col: Color
		match (i / 50) % 3:
			0: col = H("#d42028")
			1: col = Color.WHITE
			_: col = H("#2a4090")
		var tri: PackedVector2Array = PackedVector2Array([
			Vector2(i, 25), Vector2(i + 30, 25), Vector2(i + 15, 48)
		])
		draw_colored_polygon(tri, col)
	# City silhouette
	for i in range(12):
		var x: float = i * 82 + 20
		var h: float = 100 + (i*37) % 80
		draw_rect(Rect2(x, GROUND - h, 60, h), H("#0a0510"))
		var wy: float = GROUND - h + 10
		while wy < GROUND - 10:
			var wx: float = x + 8
			while wx < x + 52:
				if (int(wx + wy + i)) % 31 < 12:
					draw_rect(Rect2(wx, wy, 4, 6), H("#ffd040"))
				wx += 12
			wy += 16
	# Fireworks bursts
	var bursts: Array = [
		[200, 170, "#ff4040", 0],
		[500, 110, "#80c0ff", 35],
		[800, 140, "#ffd040", 70],
		[350, 90,  "#ffffff", 105],
		[700, 200, "#40dd40", 140],
		[120, 100, "#ff80c0", 175],
	]
	for b in bursts:
		var cycle: int = (frame + b[3]) % 200
		if cycle < 35:
			var r: float = cycle * 2.8
			var fade: float = 1.0 - cycle / 35.0
			# Center flash
			var cr: float = max(0.0, 12.0 - cycle / 3.0)
			if cr > 0:
				var fc: Color = Color.WHITE; fc.a = fade
				draw_circle(Vector2(b[0], b[1]), cr, fc)
			# Sparks
			var spark_color: Color = H(b[2]); spark_color.a = fade
			for a_idx in range(12):
				var ang: float = a_idx * PI / 6.0
				draw_line(
					Vector2(b[0] + cos(ang) * (r * 0.4), b[1] + sin(ang) * (r * 0.4)),
					Vector2(b[0] + cos(ang) * r, b[1] + sin(ang) * r),
					spark_color, 2.0
				)
			# Embers
			for a_idx in range(16):
				var ang2: float = a_idx * PI / 8.0 + cycle * 0.05
				var er: float = r * 0.85
				draw_rect(Rect2(b[0] + cos(ang2)*er - 1, b[1] + sin(ang2)*er - 1, 2, 2), spark_color)
	# Floor
	draw_rect(Rect2(0, GROUND, W, H - GROUND), H("#2a2030"))
	for i in range(0, W, 40):
		draw_rect(Rect2(i, GROUND, 2, H - GROUND), H("#1a1020"))


# ============================================================
# DRAW: FIGHTER SPRITE
# ============================================================
func _draw_fighter_sprite(cx: float, cy: float, char_def: Dictionary, facing: int, f_state: String,
		state_timer: int, scale: float, active_attack, walk_phase: float, flash_white: bool, poisoned: bool) -> void:
	var dir: int = facing
	var primary: Color = Color.WHITE if flash_white else H(char_def.primary)
	var accent: Color  = Color.WHITE if flash_white else H(char_def.accent)
	var skin: Color    = Color.WHITE if flash_white else H(char_def.skin)
	var dark: Color    = Color(0.8, 0.8, 0.8) if flash_white else H("#1a0a10")

	# Build proportions
	var BW: float
	var BH: float
	var HEAD_R: float
	match char_def.build:
		"huge":  BW = 36.0; BH = 50.0; HEAD_R = 18.0
		"broad": BW = 30.0; BH = 44.0; HEAD_R = 14.0
		"small": BW = 20.0; BH = 38.0; HEAD_R = 11.0
		_:       BW = 24.0; BH = 44.0; HEAD_R = 14.0
	BW *= scale; BH *= scale; HEAD_R *= scale
	var ARM_W: float = 8 * scale
	var ARM_L: float = 22 * scale
	var LEG_W: float = 10 * scale
	var LEG_L: float = 28 * scale

	# Animation offsets
	var crouch_offset: float = 0
	var arm_swing_l: float = 0
	var arm_swing_r: float = 0
	var leg_swing_l: float = 0
	var leg_swing_r: float = 0
	var body_tilt: float = 0
	var punch_extend: float = 0
	var kick_extend: float = 0

	if f_state == "walk":
		leg_swing_l = sin(walk_phase) * 8 * scale
		leg_swing_r = -leg_swing_l
		arm_swing_l = -leg_swing_l * 0.5
		arm_swing_r = leg_swing_l * 0.5
	elif f_state == "crouch":
		crouch_offset = 18 * scale
	elif f_state == "jump":
		arm_swing_l = -12 * scale
		arm_swing_r = 12 * scale
		leg_swing_l = -8 * scale
		leg_swing_r = -8 * scale
	elif f_state == "fall":
		arm_swing_l = -6 * scale
		arm_swing_r = 6 * scale
	elif f_state == "punch" and active_attack != null:
		var t: float = 1.0 - state_timer / float(active_attack.frames)
		var ex: float = sin(t * PI)
		punch_extend = ex * 28 * scale
		body_tilt = ex * 0.1
	elif f_state == "kick" and active_attack != null:
		var t2: float = 1.0 - state_timer / float(active_attack.frames)
		var ex2: float = sin(t2 * PI)
		kick_extend = ex2 * 38 * scale
		body_tilt = -ex2 * 0.05
	elif f_state == "block":
		arm_swing_l = 6 * scale
		arm_swing_r = -6 * scale
	elif f_state == "hit":
		body_tilt = -dir * 0.15
		arm_swing_l = -10 * scale
		arm_swing_r = 10 * scale
	elif f_state == "ko":
		_draw_ko(cx, cy, char_def, scale)
		return

	# Apply transform: translate, rotate (tilt), scale (facing)
	var xform := Transform2D()
	xform = xform.translated(Vector2(cx, cy - crouch_offset))
	if body_tilt != 0:
		xform = xform.rotated(body_tilt * dir)
	xform = xform.scaled(Vector2(dir, 1))
	draw_set_transform_matrix(xform)

	# === Legs ===
	# Back leg
	draw_rect(Rect2(-LEG_W - 3, -LEG_L + leg_swing_r, LEG_W, LEG_L), dark)
	draw_rect(Rect2(-LEG_W - 3 + 1, -LEG_L + leg_swing_r + 1, LEG_W - 2, LEG_L - 4), primary)
	draw_rect(Rect2(-LEG_W - 5, -2 + leg_swing_r, LEG_W + 4, 4), dark)
	# Front leg
	draw_rect(Rect2(3, -LEG_L + leg_swing_l, LEG_W, LEG_L), dark)
	draw_rect(Rect2(3 + 1, -LEG_L + leg_swing_l + 1, LEG_W - 2, LEG_L - 4), primary)
	draw_rect(Rect2(1, -2 + leg_swing_l, LEG_W + 4, 4), dark)
	# Kick extension
	if kick_extend > 0:
		draw_rect(Rect2(LEG_W, -LEG_L * 0.4, kick_extend, LEG_W), dark)
		draw_rect(Rect2(LEG_W, -LEG_L * 0.4 + 1, kick_extend, LEG_W - 2), primary)
		draw_rect(Rect2(LEG_W + kick_extend, -LEG_L * 0.4 - 2, 6, LEG_W + 4), dark)

	# === Body ===
	draw_rect(Rect2(-BW/2 - 1, -LEG_L - BH, BW + 2, BH + 2), dark)
	draw_rect(Rect2(-BW/2, -LEG_L - BH + 1, BW, BH), primary)
	# Belt
	draw_rect(Rect2(-BW/2, -LEG_L - 10, BW, 4), accent)
	# Chest emblem
	_draw_chest_detail(char_def, BW, BH, LEG_L)

	# === Arms ===
	# Back arm
	draw_rect(Rect2(-BW/2 - ARM_W + 1, -LEG_L - BH + 4 + arm_swing_l, ARM_W, ARM_L), dark)
	draw_rect(Rect2(-BW/2 - ARM_W + 2, -LEG_L - BH + 5 + arm_swing_l, ARM_W - 2, ARM_L - 2), primary)
	draw_rect(Rect2(-BW/2 - ARM_W, -LEG_L - BH + ARM_L + arm_swing_l, ARM_W + 2, 8), accent)
	# Front arm (or punch)
	if punch_extend > 0:
		draw_rect(Rect2(BW/2 - 1, -LEG_L - BH + 12, ARM_L + punch_extend, ARM_W), dark)
		draw_rect(Rect2(BW/2, -LEG_L - BH + 13, ARM_L + punch_extend, ARM_W - 2), primary)
		draw_rect(Rect2(BW/2 + ARM_L + punch_extend - 2, -LEG_L - BH + 9, 10, 12), accent)
		draw_rect(Rect2(BW/2 + ARM_L + punch_extend, -LEG_L - BH + 11, 8, 8), dark)
		# Speed lines
		var sl: Color = Color.WHITE; sl.a = 0.6
		for i in range(3):
			draw_rect(Rect2(BW/2 - 5 - i*4, -LEG_L - BH + 14 + i*2, 6, 1), sl)
	else:
		draw_rect(Rect2(BW/2 - 1, -LEG_L - BH + 4 + arm_swing_r, ARM_W, ARM_L), dark)
		draw_rect(Rect2(BW/2, -LEG_L - BH + 5 + arm_swing_r, ARM_W - 2, ARM_L - 2), primary)
		draw_rect(Rect2(BW/2 - 1, -LEG_L - BH + ARM_L + arm_swing_r, ARM_W + 2, 8), accent)

	# Special-move arm aura
	if f_state == "special" and state_timer > 0 and active_attack != null:
		var aura: Color = _aura_color_for(char_def.id)
		aura.a = 0.6 + sin(frame * 0.4) * 0.3
		draw_rect(Rect2(BW/2 + ARM_L - 4, -LEG_L - BH + 8, 14, 14), aura)
		aura.a = 0.3
		draw_rect(Rect2(BW/2 + ARM_L - 8, -LEG_L - BH + 4, 22, 22), aura)

	# === Head ===
	var headY: float = -LEG_L - BH - HEAD_R + 2
	draw_circle(Vector2(0, headY), HEAD_R + 1, dark)
	draw_circle(Vector2(0, headY), HEAD_R, skin)

	_draw_character_face(char_def, headY, HEAD_R, scale)

	# Block shield
	if f_state == "block":
		var sh: Color = H("#80c0ff"); sh.a = 0.4 + sin(frame * 0.4) * 0.2
		draw_circle(Vector2(BW/2 + 4, -LEG_L - BH/2), BH * 0.4, sh)
		var sp: Color = Color.WHITE
		for i in range(4):
			draw_rect(Rect2(BW/2 + 8 + sin(frame*0.3 + i)*4, -LEG_L - BH/2 + cos(frame*0.3+i)*BH*0.5, 2, 2), sp)

	# Reset transform
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

	# Poison aura around fighter (in world coords)
	if poisoned:
		var pc: Color = H("#80ff40"); pc.a = 0.3
		for i in range(5):
			var a: float = frame * 0.05 + i * 1.2
			draw_rect(Rect2(cx + cos(a)*30 - 2, cy - 50 + sin(a)*40 - 2, 4, 4), pc)


func _aura_color_for(id: String) -> Color:
	match id:
		"santa": return H("#ff4040")
		"easter": return H("#fff080")
		"jack": return H("#a040ff")
		"turkey": return H("#e0a428")
		"cupid": return H("#ff80a0")
		"patty": return H("#40dd40")
		"sam": return H("#80a0ff")
		"macabee": return H("#e0c860")
		_: return Color.WHITE


func _draw_ko(cx: float, cy: float, char_def: Dictionary, scale: float) -> void:
	var xform := Transform2D().translated(Vector2(cx, cy))
	draw_set_transform_matrix(xform)
	draw_rect(Rect2(-40, -16, 80, 14), H(char_def.primary))
	draw_rect(Rect2(-40, -16, 80, 3), H(char_def.accent))
	draw_circle(Vector2(-30, -8), 10, H(char_def.skin))
	# X eye
	draw_line(Vector2(-34, -12), Vector2(-26, -4), Color.BLACK, 2)
	draw_line(Vector2(-26, -12), Vector2(-34, -4), Color.BLACK, 2)
	# Stars
	for i in range(3):
		var a: float = frame * 0.05 + i * TAU / 3.0
		draw_rect(Rect2(-30 + cos(a) * 16 - 2, -25 + sin(a) * 8 - 2, 4, 4), H("#ffd040"))
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)


# ============================================================
# DRAW: CHARACTER CHEST DETAILS
# ============================================================
func _draw_chest_detail(char_def: Dictionary, BW: float, BH: float, LEG_L: float) -> void:
	var cy: float = -LEG_L - BH/2.0
	match char_def.id:
		"santa":
			draw_rect(Rect2(-BW/2, cy + 6, BW, 7), H("#1a0a05"))
			draw_rect(Rect2(-5, cy + 7, 10, 5), H("#ffd040"))
			draw_rect(Rect2(-2, cy + 8, 4, 3), H("#1a0a05"))
			draw_rect(Rect2(-2, cy - BH/3, 4, BH/2), Color.WHITE)
		"easter":
			_draw_ellipse(Vector2(0, cy), 5, 7, H("#fff080"))
			draw_rect(Rect2(-4, cy - 1, 8, 2), H("#80c0e8"))
			draw_rect(Rect2(-3, cy + 3, 6, 1), H("#80c0e8"))
			draw_rect(Rect2(-2, cy - 4, 4, 1), H("#ff80a0"))
		"jack":
			# Bat shape
			var bat: PackedVector2Array = PackedVector2Array([
				Vector2(-9, cy), Vector2(-5, cy - 4), Vector2(-1, cy),
				Vector2(0, cy - 1), Vector2(1, cy), Vector2(5, cy - 4),
				Vector2(9, cy), Vector2(7, cy + 3), Vector2(0, cy + 4), Vector2(-7, cy + 3)
			])
			draw_colored_polygon(bat, H("#1a0a25"))
		"turkey":
			for i in range(-1, 2):
				draw_circle(Vector2(i*4, cy), 2, H("#aa2030"))
			draw_rect(Rect2(-7, cy - 4, 4, 4), H("#3a8030"))
			draw_rect(Rect2(3, cy - 4, 4, 4), H("#3a8030"))
		"cupid":
			# Heart
			draw_circle(Vector2(-3, cy), 3, H("#ff4060"))
			draw_circle(Vector2(3, cy), 3, H("#ff4060"))
			var heart: PackedVector2Array = PackedVector2Array([
				Vector2(-6, cy + 1), Vector2(0, cy + 7), Vector2(6, cy + 1)
			])
			draw_colored_polygon(heart, H("#ff4060"))
		"patty":
			# Clover
			draw_circle(Vector2(-4, cy - 2), 3, H("#3aa040"))
			draw_circle(Vector2(4, cy - 2), 3, H("#3aa040"))
			draw_circle(Vector2(0, cy + 2), 3, H("#3aa040"))
			draw_rect(Rect2(-1, cy + 3, 2, 5), H("#1a4020"))
		"sam":
			# Stripes
			for i in range(3):
				draw_rect(Rect2(-BW/2, cy - 6 + i*4, BW, 2), Color.WHITE)
				draw_rect(Rect2(-BW/2, cy - 4 + i*4, BW, 2), H("#d42028"))
			# Star
			var star: PackedVector2Array = PackedVector2Array([
				Vector2(0, cy + 4), Vector2(2, cy + 7), Vector2(5, cy + 7),
				Vector2(2, cy + 9), Vector2(3, cy + 12), Vector2(0, cy + 10),
				Vector2(-3, cy + 12), Vector2(-2, cy + 9), Vector2(-5, cy + 7),
				Vector2(-2, cy + 7)
			])
			draw_colored_polygon(star, H("#ffd040"))
		"macabee":
			# Star of David (two triangles)
			var t1: PackedVector2Array = PackedVector2Array([
				Vector2(0, cy - 6), Vector2(-6, cy + 4), Vector2(6, cy + 4)
			])
			var t2: PackedVector2Array = PackedVector2Array([
				Vector2(0, cy + 6), Vector2(-6, cy - 4), Vector2(6, cy - 4)
			])
			draw_colored_polygon(t1, H(char_def.accent))
			draw_colored_polygon(t2, H(char_def.accent))


# ============================================================
# DRAW: CHARACTER FACES (the holiday mascot heads)
# ============================================================
func _draw_character_face(char_def: Dictionary, headY: float, R: float, scale: float) -> void:
	var eyeY: float = headY - 1
	var mouthY: float = headY + R * 0.4

	match char_def.id:
		"santa":
			# Floppy red hat
			var hat: PackedVector2Array = PackedVector2Array([
				Vector2(-R, headY - R + 4),
				Vector2(R, headY - R + 4),
				Vector2(R - 6, headY - R - 16)
			])
			draw_colored_polygon(hat, H(char_def.primary))
			draw_rect(Rect2(-R, headY - R, R*2, 5), Color.WHITE)
			draw_circle(Vector2(R - 6, headY - R - 16), 4, Color.WHITE)
			# Eyes
			draw_rect(Rect2(-5, eyeY, 2, 2), Color.BLACK)
			draw_rect(Rect2(3, eyeY, 2, 2), Color.BLACK)
			# Cheeks
			var ch: Color = H("#ffaa90"); ch.a = 0.7
			draw_rect(Rect2(-R + 1, eyeY + 4, 4, 3), ch)
			draw_rect(Rect2(R - 5, eyeY + 4, 4, 3), ch)
			# Beard (white half-circle below face)
			_draw_lower_beard(headY + 4, R, Color.WHITE)
			draw_rect(Rect2(-R, headY + 2, R*2, R - 2), Color.WHITE)
			draw_circle(Vector2(-R + 2, headY + R - 2), 3, Color.WHITE)
			draw_circle(Vector2(R - 2, headY + R - 2), 3, Color.WHITE)
			# Nose
			draw_circle(Vector2(0, headY + 2), 3, H("#ff5050"))
			# Mustache
			draw_rect(Rect2(-7, headY + 5, 14, 2), Color.WHITE)
		"easter":
			# Long ears
			draw_rect(Rect2(-8, headY - R - 22, 5, 26), H(char_def.primary))
			draw_rect(Rect2(3, headY - R - 22, 5, 26), H(char_def.primary))
			draw_rect(Rect2(-7, headY - R - 20, 3, 20), H("#ffd0e0"))
			draw_rect(Rect2(4, headY - R - 20, 3, 20), H("#ffd0e0"))
			# Pink nose
			var nose: PackedVector2Array = PackedVector2Array([
				Vector2(0, headY + 5), Vector2(-3, headY + 1), Vector2(3, headY + 1)
			])
			draw_colored_polygon(nose, H("#ff60a0"))
			# Eyes
			draw_rect(Rect2(-7, eyeY - 1, 4, 4), Color.BLACK)
			draw_rect(Rect2(3, eyeY - 1, 4, 4), Color.BLACK)
			draw_rect(Rect2(-6, eyeY - 1, 2, 2), Color.WHITE)
			draw_rect(Rect2(4, eyeY - 1, 2, 2), Color.WHITE)
			# Buck teeth
			draw_rect(Rect2(-3, mouthY + 3, 2, 5), Color.WHITE)
			draw_rect(Rect2(1, mouthY + 3, 2, 5), Color.WHITE)
			# Whiskers
			draw_line(Vector2(-3, mouthY + 1), Vector2(-10, mouthY), Color.BLACK, 1)
			draw_line(Vector2(-3, mouthY + 2), Vector2(-10, mouthY + 3), Color.BLACK, 1)
			draw_line(Vector2(3, mouthY + 1), Vector2(10, mouthY), Color.BLACK, 1)
			draw_line(Vector2(3, mouthY + 2), Vector2(10, mouthY + 3), Color.BLACK, 1)
		"jack":
			# Pumpkin ridges
			for i in range(-2, 3):
				var x: float = i * 5.0
				var n_segments: int = 8
				var prev: Vector2 = Vector2(x, headY - R + 2)
				for s in range(1, n_segments + 1):
					var t: float = s / float(n_segments)
					# Quadratic bezier from (x, top) via (x*0.7, mid) to (x, bottom)
					var mt: float = 1.0 - t
					var px_p: float = mt*mt*x + 2*mt*t*(x*0.7) + t*t*x
					var py_p: float = mt*mt*(headY - R + 2) + 2*mt*t*(headY) + t*t*(headY + R - 2)
					var curr: Vector2 = Vector2(px_p, py_p)
					draw_line(prev, curr, H("#aa4010"), 2)
					prev = curr
			# Stem
			draw_rect(Rect2(-3, headY - R - 5, 6, 7), H("#3a6020"))
			draw_rect(Rect2(-2, headY - R - 5, 2, 7), H("#5a8030"))
			# Triangle eyes
			var flicker: float = 0.7 + sin(frame * 0.3) * 0.3
			var ey: PackedVector2Array = PackedVector2Array([
				Vector2(-9, eyeY - 2), Vector2(-3, eyeY - 2), Vector2(-6, eyeY + 4)
			])
			draw_colored_polygon(ey, H("#ffe040"))
			var ey2: PackedVector2Array = PackedVector2Array([
				Vector2(9, eyeY - 2), Vector2(3, eyeY - 2), Vector2(6, eyeY + 4)
			])
			draw_colored_polygon(ey2, H("#ffe040"))
			var spark: Color = Color.WHITE; spark.a = flicker
			draw_rect(Rect2(-7, eyeY, 2, 1), spark)
			draw_rect(Rect2(5, eyeY, 2, 1), spark)
			# Jagged grin
			var grin: PackedVector2Array = PackedVector2Array([
				Vector2(-9, mouthY + 1),
				Vector2(-7, mouthY + 5), Vector2(-5, mouthY + 1),
				Vector2(-3, mouthY + 5), Vector2(-1, mouthY + 1),
				Vector2(1, mouthY + 5), Vector2(3, mouthY + 1),
				Vector2(5, mouthY + 5), Vector2(7, mouthY + 1),
				Vector2(9, mouthY + 5), Vector2(9, mouthY + 6),
				Vector2(-9, mouthY + 6)
			])
			draw_colored_polygon(grin, H("#ffe040"))
		"turkey":
			# Brown head
			draw_rect(Rect2(-R, headY - R, R*2, R + 2), H("#5a2a10"))
			draw_rect(Rect2(-R + 2, headY - R, 2, R), H("#3a1a05"))
			draw_rect(Rect2(R - 4, headY - R, 2, R), H("#3a1a05"))
			# Crest feathers
			for i in range(-1, 2):
				var t1: PackedVector2Array = PackedVector2Array([
					Vector2(i*5 - 2, headY - R + 2),
					Vector2(i*5, headY - R - 6),
					Vector2(i*5 + 2, headY - R + 2)
				])
				draw_colored_polygon(t1, H(char_def.accent))
			# Beak
			var beak: PackedVector2Array = PackedVector2Array([
				Vector2(0, headY + 1), Vector2(R + 5, headY + 4), Vector2(0, headY + 6)
			])
			draw_colored_polygon(beak, H("#ffc040"))
			draw_rect(Rect2(0, headY + 4, 4, 1), H("#aa7020"))
			# Wattle
			var wattle: PackedVector2Array = PackedVector2Array([
				Vector2(R - 3, headY + 2), Vector2(R + 2, headY + 9), Vector2(R - 7, headY + 9)
			])
			draw_colored_polygon(wattle, H("#cc2030"))
			# Eye
			draw_rect(Rect2(-2, eyeY - 1, 5, 5), Color.WHITE)
			draw_rect(Rect2(0, eyeY, 3, 3), Color.BLACK)
			draw_rect(Rect2(1, eyeY + 1, 1, 1), Color.WHITE)
		"cupid":
			# Curly hair
			for i in range(-2, 3):
				draw_circle(Vector2(i*4, headY - R + 2), 4, H("#ffd040"))
			draw_rect(Rect2(-R, headY - R + 2, R*2, 3), H("#ffd040"))
			draw_circle(Vector2(-R, headY - R + 8), 3, H("#ffd040"))
			draw_circle(Vector2(R, headY - R + 8), 3, H("#ffd040"))
			# Halo (just an ellipse outline)
			_draw_ellipse_outline(Vector2(0, headY - R - 5), 9, 3, H("#ffe080"), 2)
			# Big eyes
			draw_rect(Rect2(-7, eyeY - 1, 5, 5), Color.WHITE)
			draw_rect(Rect2(2, eyeY - 1, 5, 5), Color.WHITE)
			draw_rect(Rect2(-5, eyeY, 2, 3), H("#2a4080"))
			draw_rect(Rect2(4, eyeY, 2, 3), H("#2a4080"))
			draw_rect(Rect2(-4, eyeY, 1, 1), Color.WHITE)
			draw_rect(Rect2(5, eyeY, 1, 1), Color.WHITE)
			# Cheeks
			var c: Color = H("#ffaab8"); c.a = 0.7
			draw_rect(Rect2(-9, mouthY - 1, 3, 2), c)
			draw_rect(Rect2(6, mouthY - 1, 3, 2), c)
			# Smile
			draw_rect(Rect2(-2, mouthY + 1, 4, 1), H("#aa4060"))
			draw_rect(Rect2(-3, mouthY, 1, 1), H("#aa4060"))
			draw_rect(Rect2(2, mouthY, 1, 1), H("#aa4060"))
		"patty":
			# Top hat
			draw_rect(Rect2(-R - 3, headY - R, R*2 + 6, 3), H(char_def.primary))
			draw_rect(Rect2(-R + 1, headY - R - 14, R*2 - 2, 14), H(char_def.primary))
			draw_rect(Rect2(-R + 1, headY - R - 5, R*2 - 2, 4), H(char_def.accent))
			draw_rect(Rect2(-2, headY - R - 4, 4, 2), H("#1a0a05"))
			# Tiny clover
			draw_rect(Rect2(R - 4, headY - R - 12, 3, 3), H("#1a4020"))
			# Eyes
			draw_rect(Rect2(-5, eyeY, 3, 2), Color.BLACK)
			draw_rect(Rect2(2, eyeY, 3, 2), Color.BLACK)
			# Big red beard
			_draw_lower_beard(headY + 6, R, H("#cc4020"))
			draw_rect(Rect2(-R, headY + 4, R*2, R), H("#cc4020"))
			draw_rect(Rect2(-R + 2, headY + 8, 2, 3), H("#aa3010"))
			draw_rect(Rect2(R - 4, headY + 8, 2, 3), H("#aa3010"))
			# Mustache
			draw_rect(Rect2(-7, headY + 4, 14, 2), H("#cc4020"))
		"sam":
			# Tall blue top hat
			draw_rect(Rect2(-R - 2, headY - R + 2, R*2 + 4, 3), H(char_def.primary))
			draw_rect(Rect2(-R + 1, headY - R - 16, R*2 - 2, 16), H(char_def.primary))
			# Stripes
			draw_rect(Rect2(-R + 1, headY - R - 10, R*2 - 2, 4), Color.WHITE)
			draw_rect(Rect2(-R + 1, headY - R - 10, R*2 - 2, 1), H("#d42028"))
			draw_rect(Rect2(-R + 1, headY - R - 7, R*2 - 2, 1), H("#d42028"))
			# Stars
			for i in range(-1, 2):
				draw_rect(Rect2(i*5 - 1, headY - R - 9, 2, 2), H("#2a4090"))
			# Eyes
			draw_rect(Rect2(-7, eyeY, 4, 2), Color.BLACK)
			draw_rect(Rect2(3, eyeY, 4, 2), Color.BLACK)
			# Eyebrows
			draw_rect(Rect2(-8, eyeY - 3, 5, 2), Color.WHITE)
			draw_rect(Rect2(3, eyeY - 3, 5, 2), Color.WHITE)
			# Pointed goatee
			var goat: PackedVector2Array = PackedVector2Array([
				Vector2(-5, mouthY), Vector2(0, headY + R + 4), Vector2(5, mouthY)
			])
			draw_colored_polygon(goat, Color.WHITE)
			# Mustache
			draw_rect(Rect2(-6, mouthY - 1, 12, 2), Color.WHITE)
			draw_rect(Rect2(-7, mouthY - 2, 2, 1), Color.WHITE)
			draw_rect(Rect2(5, mouthY - 2, 2, 1), Color.WHITE)
		"macabee":
			# Conical helmet
			var helm: PackedVector2Array = PackedVector2Array([
				Vector2(-R, headY - R + 4),
				Vector2(0, headY - R - 12),
				Vector2(R, headY - R + 4)
			])
			draw_colored_polygon(helm, H(char_def.primary))
			draw_rect(Rect2(-R, headY - R + 2, R*2, 3), H(char_def.accent))
			# Star on helmet
			draw_rect(Rect2(-1, headY - R - 7, 2, 6), H(char_def.accent))
			draw_rect(Rect2(-3, headY - R - 5, 6, 2), H(char_def.accent))
			# Eyes
			draw_rect(Rect2(-7, eyeY, 4, 3), Color.WHITE)
			draw_rect(Rect2(3, eyeY, 4, 3), Color.WHITE)
			draw_rect(Rect2(-6, eyeY + 1, 2, 2), H("#1a0a05"))
			draw_rect(Rect2(4, eyeY + 1, 2, 2), H("#1a0a05"))
			# Dark beard
			_draw_lower_beard(headY + 4, R - 1, H("#1a0a05"))
			draw_rect(Rect2(-R + 2, headY + 2, R*2 - 4, R), H("#1a0a05"))
			draw_rect(Rect2(-5, headY + 3, 10, 2), H("#1a0a05"))


# ============================================================
# DRAWING HELPERS
# ============================================================

func H(hex: String) -> Color:
	return Color(hex)


func _stroke_rect(rect: Rect2, color: Color, w: float) -> void:
	draw_rect(rect, color, false, w)


func _stroke_circle(pos: Vector2, radius: float, color: Color, w: float) -> void:
	draw_arc(pos, radius, 0, TAU, 32, color, w)


func _draw_ellipse(pos: Vector2, rx: float, ry: float, color: Color) -> void:
	# Approximate with polygon
	var pts: PackedVector2Array = PackedVector2Array()
	var seg: int = 24
	for i in range(seg):
		var ang: float = i * TAU / seg
		pts.append(Vector2(pos.x + cos(ang) * rx, pos.y + sin(ang) * ry))
	draw_colored_polygon(pts, color)


func _draw_ellipse_outline(pos: Vector2, rx: float, ry: float, color: Color, w: float) -> void:
	var seg: int = 32
	var prev: Vector2 = Vector2(pos.x + rx, pos.y)
	for i in range(1, seg + 1):
		var ang: float = i * TAU / seg
		var curr: Vector2 = Vector2(pos.x + cos(ang) * rx, pos.y + sin(ang) * ry)
		draw_line(prev, curr, color, w)
		prev = curr


func _draw_semi_circle_top(center: Vector2, r: float, color: Color) -> void:
	# Half-circle facing up; use polygon
	var pts: PackedVector2Array = PackedVector2Array()
	pts.append(Vector2(center.x - r, center.y))
	var seg: int = 16
	for i in range(seg + 1):
		var ang: float = PI + i * PI / seg
		pts.append(Vector2(center.x + cos(ang) * r, center.y + sin(ang) * r))
	pts.append(Vector2(center.x + r, center.y))
	draw_colored_polygon(pts, color)


func _draw_lower_beard(y: float, r: float, color: Color) -> void:
	# Half-circle below face
	var pts: PackedVector2Array = PackedVector2Array()
	var seg: int = 16
	for i in range(seg + 1):
		var ang: float = i * PI / seg
		pts.append(Vector2(cos(ang) * r, y + sin(ang) * r))
	draw_colored_polygon(pts, color)


func _draw_v_gradient(rect: Rect2, top: Color, bottom: Color, bands: int = 16) -> void:
	for i in range(bands):
		var t: float = i / float(bands - 1)
		var c: Color = top.lerp(bottom, t)
		var y: float = rect.position.y + rect.size.y * i / float(bands)
		var hh: float = rect.size.y / float(bands) + 1
		draw_rect(Rect2(rect.position.x, y, rect.size.x, hh), c)


func _draw_centered_text(text: String, y: float, font_size: int, color: Color) -> void:
	var size: Vector2 = ui_font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size)
	draw_string(ui_font, Vector2((W - size.x) / 2.0, y), text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color)


func _draw_centered_text_at(text: String, x: float, y: float, font_size: int, color: Color) -> void:
	var size: Vector2 = ui_font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size)
	draw_string(ui_font, Vector2(x - size.x / 2.0, y), text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color)


# ============================================================
# AUDIO
# ============================================================
func _setup_audio() -> void:
	# Create pool of audio players
	for i in range(SFX_PLAYER_COUNT):
		var player := AudioStreamPlayer.new()
		add_child(player)
		sfx_players.append(player)
	# Pre-generate procedural SFX
	sfx_streams = {
		"hit":     _make_sfx(220, 0.06, "square", 0.4, -100),
		"heavy":   _make_sfx(120, 0.15, "saw",    0.5, -80),
		"block":   _make_sfx(800, 0.04, "square", 0.3, 0),
		"whoosh":  _make_noise(0.08, 0.3),
		"jump":    _make_sfx(300, 0.1,  "square", 0.3, 200),
		"fire":    _make_sfx(400, 0.2,  "saw",    0.4, -200),
		"ko":      _make_sfx(200, 0.5,  "saw",    0.5, -180),
		"select":  _make_sfx(660, 0.05, "square", 0.3, 0),
		"confirm": _make_sfx(880, 0.12, "square", 0.4, 200),
	}


func _play_sfx(name: String) -> void:
	if not sfx_streams.has(name):
		return
	var player: AudioStreamPlayer = sfx_players[sfx_index]
	sfx_index = (sfx_index + 1) % SFX_PLAYER_COUNT
	player.stream = sfx_streams[name]
	player.play()


func _make_sfx(freq: float, duration: float, shape: String, volume: float, slide: float) -> AudioStreamWAV:
	var sample_rate: int = 22050
	var sample_count: int = int(sample_rate * duration)
	var data: PackedByteArray = PackedByteArray()
	data.resize(sample_count * 2)
	var phase: float = 0.0
	for i in range(sample_count):
		var t: float = float(i) / sample_rate
		var f: float = freq + slide * t
		phase += f / sample_rate
		var v: float
		match shape:
			"square": v = 1.0 if fmod(phase, 1.0) < 0.5 else -1.0
			"saw":    v = 2.0 * fmod(phase, 1.0) - 1.0
			_:        v = sin(phase * TAU)
		# Fade out envelope
		v *= (1.0 - float(i) / sample_count) * volume
		var sample: int = int(clamp(v * 32767.0, -32767.0, 32767.0))
		# Little-endian 16-bit
		data[i * 2]     = sample & 0xff
		data[i * 2 + 1] = (sample >> 8) & 0xff
	var stream := AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = sample_rate
	stream.stereo = false
	stream.data = data
	return stream


func _make_noise(duration: float, volume: float) -> AudioStreamWAV:
	var sample_rate: int = 22050
	var sample_count: int = int(sample_rate * duration)
	var data: PackedByteArray = PackedByteArray()
	data.resize(sample_count * 2)
	for i in range(sample_count):
		var v: float = (randf() * 2.0 - 1.0) * (1.0 - float(i) / sample_count) * volume
		var sample: int = int(clamp(v * 32767.0, -32767.0, 32767.0))
		data[i * 2]     = sample & 0xff
		data[i * 2 + 1] = (sample >> 8) & 0xff
	var stream := AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = sample_rate
	stream.stereo = false
	stream.data = data
	return stream
