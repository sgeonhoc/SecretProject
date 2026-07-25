# 조명 값들을 하루 세 때 프리셋 변수로 갈아 끼운다(일회성 패치).
import io

p = "C:/Secret_Project/_build_L01_v2.py"
s = io.open(p, encoding="utf-8").read()
rep = [
    ('dl.set_actor_rotation(unreal.Rotator(0.0, -34.0, -62.0), False)',
     'dl.set_actor_rotation(unreal.Rotator(0.0, p_pitch, p_yaw), False)'),
    ('lc.set_intensity(6.5)', 'lc.set_intensity(p_lux)'),
    ('lc.set_light_color(unreal.LinearColor(1.0, 0.80, 0.56))',
     'lc.set_light_color(unreal.LinearColor(*p_col))'),
    ('dl.set_actor_label("\ud574 (\uc800\ub141)")',
     'dl.set_actor_label("\ud574 (%s)" % PHASE)'),
    ('fc.set_editor_property("fog_density", 0.008)',
     'fc.set_editor_property("fog_density", p_fog)'),
    ('fc.set_editor_property("fog_inscattering_luminance", '
     'unreal.LinearColor(0.30, 0.25, 0.22))',
     'fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(*p_fogcol))'),
    ('skc.set_editor_property("intensity", 2.6)',
     'skc.set_editor_property("intensity", p_sky)'),
    ('s.set_editor_property("auto_exposure_bias", 0.6)',
     's.set_editor_property("auto_exposure_bias", p_exp)'),
    ('c.set_intensity(2600.0)', 'c.set_intensity(p_lamp)'),
]
for a, b in rep:
    if a not in s:
        print("!! 못 찾음: " + a[:70])
        continue
    s = s.replace(a, b, 1)
io.open(p, "w", encoding="utf-8").write(s)
print("치환 완료")
