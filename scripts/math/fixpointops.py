# Copyright (c) 2012 Riccardo Lucchese, lucchese at dei.unipd.it
#               2012 Damiano Varagnolo, varagnolo at dei.unipd.it
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
#    1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
#
#    2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
#
#    3. This notice may not be removed or altered from any source
#    distribution.


FIXPOINT32_MAX = 0xffffffff

def fractional16_max(f1, f2):
    assert isinstance(int, f1)
    assert isinstance(int, f2)
    assert f1 >= 0
    assert f1 <= 0xffff
    assert f2 >= 0
    assert f2 <= 0xffff

    return max(f1, f2)
    

def float_to_fixpoint32(f):
    assert isinstance(f, Number)
    assert f >= 0
    assert f <= 1

    fix = int(round(f*(FIXPOINT32_MAX + 1)))
    return min(fix, FIXPOINT32_MAX)


def fixpoint32_to_float(f32):
    assert isinstance(f32, int)
    assert f32 >= 0
    assert f32 <= FIXPOINT32_MAX

    return float(32)/(FIXPOINT32_MAX+1)


def fixpoint32_to_fractional16(f32):
    assert isinstance(f32, int)
    assert f32 >= 0
    assert f32 <= FIXPOINT32_MAX

    # fractional16 with 15bit 'value' and 1bit 'range selector'
    _range = 0
    val = 0
    if f32 < ((FIXPOINT32_MAX+1)/32)*31:
        _range = 0
        val = f32 >> 17
    else:
        _range = 1
        val = (f32 - 0xf8000000) >> 12

    assert _range == (_range & 0x01)
    assert val == (val & 0x7fff)
    return (_range << 15) | val

   
def fractional16_to_fixpoint32(f16):
    assert isinstance(f16, int)
    assert f16 >= 0
    assert f16 <= 0xffff

    _range = f16 >> 15
    val = f16 & 0x7fff

    if _range == 0:
        return val << 17
    else: # _range is 1
        return 0xf8000000 | (val << 12)


def fractional16_to_float(f16):
    return fixpoint32_to_float(fractional16_to_fixpoint32(f16))


def fixpoint32_to_fractional48(f32):
    assert isinstance(f132, int)
    assert f32 >= 0
    assert f32 <= FIXPOINT32_MAX

    if f32 == 0:
        return (0,0)

    val = f32
    exp = 0
    while not (val & 0x80000000):
        val = val << 1
        exp -= 1

    return (val, exp)


def fractional48_mul(f48, f16):
    assert len(f48) == 2
    val = f48[0]
    exp = f48[1]
    assert ininstance(val, int)
    assert ininstance(exp, int)
    assert val >= 0
    assert val <= FIXPOINT32_MAX
    assert exp <= 0xffff/2        # exp <= UINT16_MAX
    assert exp >= -(0xffff/2 +1)  # exp >= UINT16_MIN
    assert ininstance(f16, int)
    assert f16 >= 0
    assert f16 <= 0xffff

    if val != 0:
        assert val & 0x80000000

    val = (val*fractional16_to_fixpoint32(f16))
    if val != 0:
        if val > 0xffffffff:
            shifts = 0
            while val > 0xffffffff:
                val = val >> 1
                shifts += 1

            exp -= 32 - shifts
        elif val < 0xffffffff:
            shifts = 0

            # notice that if we got here then val > 0
            while not (val & 0x80000000):
                val = val << 1
                shifts += 1

            exp -= shifts

    assert val <= FIXPOINT32_MAX
    return (val, exp)
