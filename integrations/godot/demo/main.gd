extends Control
## Книга NLB на экране Godot: NLBPlayer открывает книгу, NLBScreen показывает её так же, как
## nlb_sdl и Unreal (общий GameScreen: тема INSTEAD, VN, экран вариантов, меню по Esc).
## Свой интерфейс поверх get_screen() — пример в custom_ui_example.gd.

@onready var player: NLBPlayer = $Player
@onready var screen: NLBScreen = $Screen

func _ready() -> void:
	player.achievement.connect(func(name): print("Достижение: ", name))
	player.external_game.connect(_on_external_game)
	screen.quit_requested.connect(func(): get_tree().quit())

func _on_external_game(name: String) -> void:
	# Здесь игра передаёт управление своей сцене (мини-игра, 3D). Пример: «кубик» даёт 100 монет
	print("Внешняя игра: ", name)
	player.set_variable("money", int(player.get_variable("money")) + 100)
	player.finish_external_game()
