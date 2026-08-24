//
// BSGL Tutorial 10 - written entirely in Wren
//
// The host (main.cpp) only calls bsglWren.Run("main.wren"); this
// script does the rest. It must define the global `var main` with
// init() (resources are created here, after the system is up),
// update(dt) (logic; return true to quit) and render().
//

import "bsgl" for System, Timer, Input, Key, Color, Anim, Gfx
import "bsgl" for Texture, Sprite, Animation, Font, Spine, DBones

class Main {
    construct new() {
        // nothing here yet - resources need the renderer, use init()
    }

    init() {
        _t = 0
        _x = 400.0

        // static sprite
        var tex = Texture.new("mushroom.bmp")
        if (tex.loaded) {
            _mushroom = Sprite.new(tex, 0, 0, 64, 64)
            _mushroom.setHotSpot(32, 32)
        } else {
            _mushroom = null
        }

        // frame animation (6 frames of 64x64, 12 fps)
        var animTex = Texture.new("anim.bmp")
        if (animTex.loaded) {
            _anim = Animation.new(animTex, 6, 12, 0, 0, 64, 64)
            _anim.setMode(Anim.loop)
            _anim.setHotSpot(32, 32)
            _anim.play
        } else {
            _anim = null
        }

        // font for the FPS counter
        _font = Font.new("font.ttf", 16)

        // optional skeletal animations (guarded by loaded)
        _spine = Spine.new("spineboy-ess.json", "spineboy.atlas")
        if (_spine.loaded) {
            _spine.setPos(600, 500)
            _spine.setScale(0.4, 0.4)
            _spine.setDefaultMix(0.2)
            _spine.setAnimation(0, "walk", true)
        }

        _dbones = DBones.new("DBMecha_ske.json", "DBMecha_tex.json", "DBMecha_tex.bmp")
        if (_dbones.loaded) {
            _dbones.setPos(200, 520)
            _dbones.play("idle")
        }
    }

    update(dt) {
        if (Input.isDown(Key.esc)) {
            return true
        }

        _t = _t + dt

        // move the mushroom with the arrow keys
        var speed = 200 * dt
        if (Input.isDown(Key.left))  _x = _x - speed
        if (Input.isDown(Key.right)) _x = _x + speed

        if (_anim) _anim.update(dt)
        if (_spine.loaded) _spine.update(dt)
        if (_dbones.loaded) _dbones.update(dt)

        return false
    }

    render() {
        Gfx.beginScene
        Gfx.clear(Color.rgba(0x20, 0x20, 0x40, 0xFF))

        if (_mushroom) _mushroom.renderEx(_x, 150, _t)
        if (_anim) _anim.render(400, 300)
        if (_spine.loaded) _spine.render
        if (_dbones.loaded) _dbones.render

        _font.render(5, 5, "FPS: %(Timer.fps)", Color.white)

        Gfx.endScene
    }
}

System.title = "BSGL Tutorial 10 - Wren scripting"
System.windowed = true
System.width = 800
System.height = 600

var main = Main.new()
