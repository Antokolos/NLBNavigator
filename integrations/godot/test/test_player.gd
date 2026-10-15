extends SceneTree
# Проверка NLBPlayer в настоящем Godot (без окна): книга, снимок экрана, щелчки, внешняя игра

func _initialize() -> void:
	var p = ClassDB.instantiate("NLBPlayer")
	if p == null:
		print("FAIL: класс NLBPlayer не зарегистрирован")
		quit(1)
		return
	p.autostart = false
	p.book_path = "res://book.nlbpack"
	p.external_games = PackedStringArray(["play_bp_*"])
	root.add_child(p)
	var got_external := [false]
	p.external_game.connect(func(n): got_external[0] = true; print("external_game: ", n))
	print("open: ", p.open())
	var s: Dictionary = p.get_screen()
	print("layout: ", s.layout, ", объектов: ", s.objects.size(), ", вариантов: ", s.choices.size())
	var tex = p.load_texture(s.page_image.frame_path)
	print("фон: ", s.page_image.frame_path, " -> ", tex.get_size() if tex else "нет")
	for label in ["Грамотность", "Распознавание", "Воровство"]:
		for o in p.get_screen().objects:
			if str(o.label).begins_with(label):
				p.click_object(o.id)
	for c in [0, 0, 0, 0, 1, 0, 1, 0, 0]:
		p.choose(c)
	s = p.get_screen()
	print("внешняя игра в снимке: ", s.external_game, ", сигнал: ", got_external[0])
	p.set_variable("money", int(p.get_variable("money")) + 100)
	print("finish: ", p.finish_external_game(), ", money = ", p.get_variable("money"))
	var text := ""
	for para in p.get_screen().page_text:
		for run in para:
			text += run.text
	print("текст после возврата: ", text.substr(0, 60))
	quit(0)
