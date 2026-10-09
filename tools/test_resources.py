#!/usr/bin/env python3
"""Проверяет ресурсы Injoer: шрифт, шейдеры, звук прыжка, отсутствие старой игры."""

from __future__ import annotations

import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def check_assets() -> None:
    required = (
        ROOT / "assets" / "fonts" / "ComicRelief-Regular.ttf",
        ROOT / "assets" / "shaders" / "sprite.vert",
        ROOT / "assets" / "shaders" / "solid.frag",
        ROOT / "assets" / "shaders" / "image.frag",
        ROOT / "assets" / "shaders" / "tint.frag",
        ROOT / "assets" / "audio" / "jump.wav",
    )
    for asset in required:
        assert asset.is_file(), f"Не найден ресурс: {asset.relative_to(ROOT)}"
    with wave.open(str(ROOT / "assets" / "audio" / "jump.wav"), "rb") as audio:
        assert audio.getnchannels() in (1, 2), "jump.wav: жду моно или стерео"
        assert audio.getsampwidth() == 2, "jump.wav: жду 16 бит"
        assert audio.getframerate() in (22050, 44100, 48000), "jump.wav: странная частота"


def check_new_game() -> None:
    game = ROOT / "src" / "game"
    for name in ("game.c", "game.h", "math3d.inc", "world.inc", "render3d.inc", "input.inc"):
        assert (game / name).is_file(), f"Нет файла новой игры: {name}"
    assert (ROOT / "src" / "engine" / "graphics" / "tri.inc").is_file(), "Нет tri.inc"
    for old in ("combat", "core", "fx", "include", "roblox", "state", "ui"):
        assert not (game / old).exists(), f"Остатки старой игры: src/game/{old}"
    for old in ("objects.inc", "types.inc", "functions.inc", "lifecycle.inc", "state.inc"):
        assert not (game / old).exists(), f"Остаток старой игры: src/game/{old}"
    assert not (ROOT / "assets" / "textures").exists(), "Остатки старой игры: assets/textures"
    assert not (ROOT / "FIREBASE.md").exists(), "Остаток онлайна: FIREBASE.md"
    assert not (ROOT / "firebase.rules.json").exists(), "Остаток онлайна: firebase.rules.json"


def check_port_layout() -> None:
    assert not list(ROOT.rglob("*.ds")), "В C-порте остались исходники старого языка"
    assert not (ROOT / "game").exists(), "Старый каталог game не должен использоваться"
    assert not (ROOT / "native").exists(), "Старый каталог native не должен использоваться"


def main() -> int:
    check_assets()
    check_new_game()
    check_port_layout()
    print("Ресурсы Injoer: норма")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
