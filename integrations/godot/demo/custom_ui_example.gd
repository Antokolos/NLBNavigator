extends Control
## Пример своего интерфейса поверх NLBPlayer.get_screen() (без NLBScreen): подключите этот
## скрипт к узлу Control вместо main.gd, а узел Screen удалите
## Простейший интерфейс поверх NLBPlayer: картинка/фон, графические объекты, текст со ссылками,
## варианты, инвентарь. Положение объектов — по правилам vn.lua (см. ScreenModel.h).
## Это пример: настоящая игра рисует снимок экрана своими средствами (UI, 3D, что угодно).

@onready var player: NLBPlayer = $Player
var background := TextureRect.new()
var objects_layer := Control.new()
var text := RichTextLabel.new()
var inventory := HBoxContainer.new()
const SCREEN := Vector2(1920, 1080)

func _ready() -> void:
	for n in [background, objects_layer, text, inventory]:
		add_child(n)
	background.set_anchors_preset(Control.PRESET_FULL_RECT)
	background.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	background.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	objects_layer.set_anchors_preset(Control.PRESET_FULL_RECT)
	objects_layer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	text.position = Vector2(248, 872)
	text.size = Vector2(1424, 168)
	text.bbcode_enabled = true
	text.meta_clicked.connect(_on_meta)
	inventory.position = Vector2(0, 1050)
	player.screen_changed.connect(_redraw)
	player.achievement.connect(func(name): print("Достижение: ", name))
	player.external_game.connect(_on_external_game)
	_redraw()

func _redraw() -> void:
	var s: Dictionary = player.get_screen()
	if s.is_empty():
		return
	background.texture = player.load_texture(s.page_image.frame_path)
	_draw_objects(s)
	var bb := ""
	for paragraph in s.page_text + s.reaction_text:
		for run in paragraph:
			if run.kind == "object":
				bb += "[url=obj:%s]%s[/url]" % [run.target, run.text]
			else:
				bb += run.text
		bb += "\n"
	var n := 0
	for c in s.choices:
		if c.enabled:
			n += 1
			bb += "[url=choice:%d]%d. %s[/url]\n" % [c.index, n, c.text]
	text.text = bb
	for child in inventory.get_children():
		child.queue_free()
	for item in s.inventory:
		var b := Button.new()
		b.text = item.label
		b.disabled = not item.active
		b.toggle_mode = true
		b.button_pressed = item.id == s.selected_item
		b.pressed.connect(func(): player.click_item(item.id))
		inventory.add_child(b)

func _draw_objects(s: Dictionary) -> void:
	for child in objects_layer.get_children():
		child.queue_free()
	var rects := {}
	var stacked := {}
	for o in s.objects:
		if not o.graphical or not o.visible or o.collapsed:
			continue
		var tex: Texture2D = player.load_texture(o.image.frame_path)
		var size: Vector2 = tex.get_size() if tex else Vector2.ZERO
		var pos := Vector2.ZERO
		if o.parent_id != "" and rects.has(o.parent_id):
			# вложенный: позиция родителя + iarm, иначе столбиком под предыдущими детьми
			var arm: Vector2 = o.inner_arm if o.has_inner_arm else Vector2(0, stacked.get(o.parent_id, 0.0))
			if not o.has_inner_arm:
				stacked[o.parent_id] = stacked.get(o.parent_id, 0.0) + size.y
			pos = rects[o.parent_id].position + arm
		else:
			var a: String = o.anchor
			pos.x = o.arm.x if a.begins_with("left") else (SCREEN.x - size.x if a.begins_with("right") else floor((SCREEN.x - size.x) / 2))
			pos.y = o.arm.y if a.ends_with("top") else (SCREEN.y - size.y if a.ends_with("bottom") else floor((SCREEN.y - size.y) / 2))
			if o.has_offset:
				pos += o.offset
		rects[o.id] = Rect2(pos, size)
		var sprite := TextureButton.new()
		sprite.texture_normal = tex
		sprite.texture_hover = player.load_texture(o.hover_image.frame_path) if o.hover_image.frame_path != "" else tex
		sprite.position = pos
		sprite.disabled = not o.active
		sprite.pressed.connect(func(): player.click_object(o.id))
		objects_layer.add_child(sprite)
		if o.effect == "fadein":
			sprite.modulate.a = 0.0
			create_tween().tween_property(sprite, "modulate:a", 1.0, max(o.max_step - o.cur_step, 1) * 0.02)

func _on_meta(meta) -> void:
	var m := str(meta)
	if m.begins_with("obj:"):
		player.click_object(m.substr(4))
	elif m.begins_with("choice:"):
		player.choose(int(m.substr(7)))

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and event.keycode >= KEY_1 and event.keycode <= KEY_9:
		var k: int = event.keycode - KEY_1
		var n := 0
		for c in player.get_screen().choices:
			if c.enabled:
				if n == k:
					player.choose(c.index)
					return
				n += 1
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_RIGHT:
		player.clear_selection()

func _on_external_game(name: String) -> void:
	# Здесь игра передаёт управление своей сцене (мини-игра, 3D). Пример: «кубик» даёт 100 монет
	print("Внешняя игра: ", name)
	player.set_variable("money", int(player.get_variable("money")) + 100)
	player.finish_external_game()
