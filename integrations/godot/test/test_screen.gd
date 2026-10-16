extends SceneTree
# Проверка NLBScreen в Godot без окна: «Варвар», режим экрана вариантов — пробел проходит
# страницы с единственной «Далее» сам и останавливается на настоящем выборе

var player
var screen
var frames := 0
var presses := 0
var pages := []

func _initialize() -> void:
	player = ClassDB.instantiate("NLBPlayer")
	player.autostart = false
	player.book_path = "res://book.nlbpack"
	player.external_games = PackedStringArray(["play_bp_*"])
	root.add_child(player)
	screen = ClassDB.instantiate("NLBScreen")
	screen.choices_screen = true
	screen.size = Vector2(1280, 720)
	root.add_child(screen)
	screen.player = screen.get_path_to(player)
	print("open: ", player.open())
	for label in ["Грамотность", "Распознавание", "Воровство"]:
		for o in player.get_screen().objects:
			if str(o.label).begins_with(label):
				player.click_object(o.id)
	player.choose(0)

func _process(_delta: float) -> bool:
	frames += 1
	if frames < 3:
		return false
	var page = player.get_screen().page_id
	if pages.is_empty() or pages[-1] != page:
		pages.append(page)
	if screen.is_choices_screen_shown() or presses >= 60:
		var enabled := 0
		for c in player.get_screen().choices:
			if c.enabled:
				enabled += 1
		print("нажатий: ", presses, ", пройдено страниц: ", pages.size(), ", вариантов: ", enabled,
			", экран вариантов: ", screen.is_choices_screen_shown(), ", команд рисования: ", screen.get_draw_count())
		print("РЕЗУЛЬТАТ: ", "OK" if screen.is_choices_screen_shown() and pages.size() >= 3 and enabled >= 2 else "FAIL")
		return true
	screen.next_page()
	presses += 1
	return false
