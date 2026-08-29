//
// bsgl embedded script library (module "bsgl")
//
// Injected by bsglWren as the "bsgl" module. Foreign methods are
// implemented in src/advance/bsglwren.cpp; everything below them is
// plain Wren sugar on top of the raw bindings.
//

class Blend {
    static colorMul    { 0 }
    static colorAdd    { 1 }
    static alphaBlend  { 2 }
    static zWrite      { 4 }
    static default     { 2 }
    static defaultZ    { 6 }
}

class Color {
    static rgba(r, g, b, a) { (a * 16777216 + b * 65536 + g * 256 + r).truncate }
    static white  { 0xFFFFFFFF }
    static black  { 0xFF000000 }
    static red    { 0xFF0000FF }
    static green  { 0xFF00FF00 }
    static blue   { 0xFFFF0000 }
}

class Key {
    static mouseL    { 0 }
    static mouseR    { 1 }
    static a { 65 }
    static b { 66 }
    static c { 67 }
    static d { 68 }
    static e { 69 }
    static f { 70 }
    static g { 71 }
    static h { 72 }
    static i { 73 }
    static j { 74 }
    static k { 75 }
    static l { 76 }
    static m { 77 }
    static n { 78 }
    static o { 79 }
    static p { 80 }
    static q { 81 }
    static r { 82 }
    static s { 83 }
    static t { 84 }
    static u { 85 }
    static v { 86 }
    static w { 87 }
    static x { 88 }
    static y { 89 }
    static z { 90 }
    static d0 { 110 }
    static d1 { 101 }
    static d2 { 102 }
    static d3 { 103 }
    static d4 { 104 }
    static d5 { 105 }
    static d6 { 106 }
    static d7 { 107 }
    static d8 { 108 }
    static d9 { 109 }
    static f1 { 121 }
    static f2 { 122 }
    static f3 { 123 }
    static f4 { 124 }
    static f5 { 125 }
    static f6 { 126 }
    static f7 { 127 }
    static f8 { 128 }
    static f9 { 129 }
    static f10 { 130 }
    static f11 { 131 }
    static f12 { 132 }
    static esc      { 160 }
    static tab      { 161 }
    static capsLock { 162 }
    static shiftL   { 163 }
    static shiftR   { 164 }
    static ctrlL    { 165 }
    static ctrlR    { 166 }
    static altL     { 167 }
    static altR     { 168 }
    static space    { 169 }
    static enter    { 170 }
    static del      { 171 }
    static up       { 181 }
    static down     { 182 }
    static left     { 183 }
    static right    { 184 }
    static home     { 191 }
    static end      { 192 }
    static pgUp     { 193 }
    static pgDn     { 194 }
    static touch    { 255 }
}

class Anim {
    static fwd          { 0 }
    static rev          { 1 }
    static pingpong     { 2 }
    static loop         { 4 }
    static pingpongLoop { 6 }
}

// Axis-aligned rectangle, mirrors bsglRect
class Rect {
    construct new(x1, y1, x2, y2) {
        _x1 = x1
        _y1 = y1
        _x2 = x2
        _y2 = y2
        _clean = false
    }

    x1 { _x1 }
    y1 { _y1 }
    x2 { _x2 }
    y2 { _y2 }

    clear() {
        _x1 = 0
        _y1 = 0
        _x2 = 0
        _y2 = 0
        _clean = true
    }
    isClean { _clean }

    set(x1, y1, x2, y2) {
        _x1 = x1
        _y1 = y1
        _x2 = x2
        _y2 = y2
        _clean = false
    }

    setRadius(x, y, r) {
        _x1 = x - r
        _y1 = y - r
        _x2 = x + r
        _y2 = y + r
        _clean = false
    }

    encapsulate(x, y) {
        if (_clean || x < _x1) _x1 = x
        if (_clean || y < _y1) _y1 = y
        if (_clean || x > _x2) _x2 = x
        if (_clean || y > _y2) _y2 = y
        _clean = false
    }

    testPoint(x, y) { x >= _x1 && x <= _x2 && y >= _y1 && y <= _y2 }

    intersect(r) {
        _x1 > r.x2 || _x2 < r.x1 || _y1 > r.y2 || _y2 < r.y1
    }
}

class Sys {
    foreign static title=(v)
    foreign static logFile=(v)
    foreign static cfgFile=(v)
    foreign static windowed=(v)
    foreign static width=(v)
    foreign static height=(v)
    foreign static width
    foreign static height
    foreign static log(msg)
    foreign static quit
    foreign static errorMessage
}

class Config {
    foreign static getInt(sec, opt, def)
    foreign static setInt(sec, opt, val)
    foreign static getFloat(sec, opt, def)
    foreign static setFloat(sec, opt, val)
    foreign static getString(sec, opt, def)
    foreign static setString(sec, opt, val)
}

class Timer {
    foreign static time
    foreign static delta
    foreign static fps
}

class Random {
    foreign static seed(s)
    foreign static int(min, max)
    foreign static float(min, max)
}

class Input {
    foreign static update
    foreign static isDown(key)
    foreign static isPassing(key)
    foreign static isUp(key)
    foreign static mouseX
    foreign static mouseY
}

foreign class Sound {
    construct new(filename) { load_(filename) }

    foreign load_(filename)
    foreign loaded
    foreign play
    foreign vol=(v)
    foreign pan=(p)
    foreign free
}

foreign class Music {
    construct new(filename) { load_(filename) }

    foreign load_(filename)
    foreign loaded
    foreign play
    foreign stop
    foreign pause
    foreign resume
    foreign vol=(v)
    foreign pan=(p)
    foreign pitch=(p)
    foreign fadeTo(ms, v)
    foreign slideTo(ms, v, p, pitch)
    foreign pos
    foreign pos=(ms)
    foreign length
    foreign free
}

class Channel {
    foreign static vol(ch, v)
    foreign static pan(ch, p)
    foreign static pitch(ch, p)
    foreign static fadeTo(ch, ms, v)
    foreign static slideTo(ch, ms, v, p, pitch)
    foreign static stop(ch)
    foreign static pause(ch)
    foreign static resume(ch)
    foreign static isPlaying(ch)
    foreign static length(ch)
}

class Gfx {
    foreign static beginScene
    foreign static beginScene(debug)
    foreign static endScene
    foreign static clear(color)
    foreign static setClipping(x, y, w, h)
    foreign static setTransform(x, y, dx, dy, rot, hscale, vscale)
    foreign static renderQuadRaw(q)

    static fillRect(color, x1, y1, x2, y2) {
        var q = Quad.new()
        q.setTexture(null)
        q.blend = Blend.default
        q.setVertex(0, 0, 0, x1, y1, 0.5, color)
        q.setVertex(1, 0, 0, x2, y1, 0.5, color)
        q.setVertex(2, 0, 0, x2, y2, 0.5, color)
        q.setVertex(3, 0, 0, x1, y2, 0.5, color)
        renderQuadRaw(q)
    }
}

foreign class Quad {
    construct new() { }

    foreign setTexture(texture)
    foreign blend=(b)
    foreign setVertex(i, tx, ty, x, y, z, color)
}

foreign class Texture {
    construct new(filename) { load_(filename) }
    construct blank(w, h)   { create_(w, h) }

    foreign load_(filename)
    foreign create_(w, h)
    foreign loaded
    foreign width
    foreign height
    foreign free
}

foreign class Sprite {
    construct new(texture, x, y, w, h) { init_(texture, x, y, w, h) }

    foreign init_(texture, x, y, w, h)

    foreign render(x, y)
    foreign renderEx(x, y, rot)
    foreign renderEx(x, y, rot, hscale, vscale)
    foreign renderStretch(x1, y1, x2, y2)
    foreign render4V(x0, y0, x1, y1, x2, y2, x3, y3)

    foreign setTexture(texture)
    foreign setTextureRect(x, y, w, h)
    foreign setColor(color)
    foreign setZ(z)
    foreign setBlendMode(blend)
    foreign setHotSpot(x, y)
    foreign setFlip(fx, fy)

    foreign width
    foreign height
}

foreign class Sprite9Slice {
    construct new(texture, x, y, w, h) { init_(texture, x, y, w, h) }

    foreign init_(texture, x, y, w, h)

    foreign setInsets(left, top, right, bottom)
    foreign render(x, y, width, height)

    foreign setTexture(texture)
    foreign setTextureRect(x, y, w, h)
    foreign setColor(color)
    foreign setZ(z)
    foreign setBlendMode(blend)

    foreign width
    foreign height
}

foreign class Animation {
    construct new(texture, nframes, fps, x, y, w, h) {
        init_(texture, nframes, fps, x, y, w, h)
    }

    foreign init_(texture, nframes, fps, x, y, w, h)

    foreign play
    foreign stop
    foreign resume
    foreign update(dt)
    foreign isPlaying

    foreign setTexture(texture)
    foreign setTextureRect(x, y, w, h)
    foreign setMode(mode)
    foreign setSpeed(fps)
    foreign setFrame(n)
    foreign setFrames(n)

    foreign mode
    foreign speed
    foreign frame
    foreign frames

    foreign render(x, y)
    foreign renderEx(x, y, rot)
    foreign renderEx(x, y, rot, hscale, vscale)
    foreign setColor(color)
    foreign setBlendMode(blend)
    foreign setHotSpot(x, y)
    foreign setFlip(fx, fy)
    foreign width
    foreign height
}

foreign class Font {
    construct new(filename, size) { init_(filename, size) }

    foreign init_(filename, size)
    foreign loaded
    foreign render(x, y, text, color)
    foreign width(text)
}

foreign class Spine {
    construct new(skeletonFile, atlasFile) { init_(skeletonFile, atlasFile) }

    foreign init_(skeletonFile, atlasFile)
    foreign loaded

    foreign setAnimation(track, name, loop)
    foreign addAnimation(track, name, loop, delay)
    foreign setEmptyAnimation(track, mixDuration)
    foreign clearTrack(track)
    foreign clearTracks
    foreign currentAnimation
    foreign isPlaying
    foreign timeScale=(v)
    foreign timeScale
    foreign setDefaultMix(duration)
    foreign setMix(fromName, toName, duration)

    foreign setPos(x, y)
    foreign x
    foreign y
    foreign rotation=(deg)
    foreign rotation
    foreign setScale(sx, sy)
    foreign setFlip(fx, fy)
    foreign color=(c)
    foreign color

    foreign update(dt)
    foreign render
    foreign hitTest(x, y)

    foreign onEvent(fn)
    foreign onComplete(fn)
}

foreign class DBones {
    construct new(skeFile, atlasFile, imgFile) {
        init_(skeFile, atlasFile, imgFile, "default", null)
    }
    construct named(skeFile, atlasFile, imgFile, dataName, armatureName) {
        init_(skeFile, atlasFile, imgFile, dataName, armatureName)
    }

    foreign init_(skeFile, atlasFile, imgFile, dataName, armatureName)
    foreign loaded

    foreign play(name)
    foreign playTimes(name, times)
    foreign fadeIn(name, fadeTime)
    foreign hasAnimation(name)
    foreign animationCount
    foreign animationName(i)
    foreign currentAnimation
    foreign timeScale=(v)
    foreign timeScale

    foreign setPos(x, y)
    foreign x
    foreign y
    foreign setScale(sx, sy)
    foreign setFlip(fx, fy)
    foreign color=(c)
    foreign color

    foreign update(dt)
    foreign render
}

foreign class GUIWidget {
    construct new(x, y, w, h) { init_(x, y, w, h) }

    foreign init_(x, y, w, h)

    foreign setX(v)
    foreign setY(v)
    foreign backgroundColor=(c)
    foreign addKid(kid)
    foreign render(x, y)
    foreign mouseAt(x, y, state)
    foreign testAt(x, y)

    static mouseDefault { 0 }
    static mouseDown    { 1 }
    static mousePassing { 2 }
    static mouseUp      { 3 }

    /* Override these in a subclass (`class Panel is GUIWidget`). */
    onRender(x, y) { }
    onOver(x, y) { }
    onDown() { }
    onMove(dx, dy) { }
    onUp(inside) { }
}

foreign class GUILabel {
    construct new(font, x, y, w, h) { init_(font, x, y, w, h) }

    foreign init_(font, x, y, w, h)

    foreign setMode(align)
    foreign setText(text)
    foreign backgroundColor=(c)    // text color
    foreign setX(v)
    foreign setY(v)
    foreign addKid(kid)
    foreign render(x, y)
    foreign mouseAt(x, y, state)
    foreign testAt(x, y)
    foreign width
    foreign height

    static alignLeft   { 0 }
    static alignCenter { 1 }
    static alignRight  { 2 }
}

foreign class GUIButton {
    construct new(x, y, w, h, texture, txUp, tyUp, txDown, tyDown) {
        init_(x, y, w, h, texture, txUp, tyUp, txDown, tyDown)
    }

    foreign init_(x, y, w, h, texture, txUp, tyUp, txDown, tyDown)

    foreign setMode(trigger)
    foreign setState(down)
    foreign state
    foreign setX(v)
    foreign setY(v)
    foreign addKid(kid)
    foreign render(x, y)
    foreign mouseAt(x, y, state)
    foreign testAt(x, y)

    /* Override in a subclass to handle clicks. */
    onClick() { }
}

foreign class GUISlider {
    construct new(x, y, w, h, texture, tx, ty, sw, sh) {
        init_(x, y, w, h, texture, tx, ty, sw, sh)
    }

    foreign init_(x, y, w, h, texture, tx, ty, sw, sh)

    foreign setMode(min, max, mode)
    foreign setValue(v)
    foreign value
    foreign setX(v)
    foreign setY(v)
    foreign addKid(kid)
    foreign render(x, y)
    foreign mouseAt(x, y, state)
    foreign testAt(x, y)

    static bar          { 0 }
    static barRelative  { 1 }
    static slider       { 2 }
}
