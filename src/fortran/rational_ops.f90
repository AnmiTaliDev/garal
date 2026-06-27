! SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
! SPDX-License-Identifier: GPL-3.0-only

module rational_ops
    use iso_c_binding
    implicit none
contains

    function rat_gcd(a, b) result(g) bind(C, name="rat_gcd")
        integer(c_int64_t), intent(in), value :: a, b
        integer(c_int64_t) :: g
        integer(c_int64_t) :: ta, tb, tmp
        ta = abs(a)
        tb = abs(b)
        if (ta == 0) then
            g = tb
            return
        end if
        if (tb == 0) then
            g = ta
            return
        end if
        do while (tb /= 0)
            tmp = tb
            tb = mod(ta, tb)
            ta = tmp
        end do
        g = ta
    end function

    subroutine rat_reduce(num, den) bind(C, name="rat_reduce")
        integer(c_int64_t), intent(inout) :: num, den
        integer(c_int64_t) :: g
        if (den == 0) return
        g = rat_gcd(abs(num), abs(den))
        if (g > 0) then
            num = num / g
            den = den / g
        end if
        if (den < 0) then
            num = -num
            den = -den
        end if
    end subroutine

    subroutine rat_add(an, ad, bn, bd, cn, cd) bind(C, name="rat_add")
        integer(c_int64_t), intent(in), value :: an, ad, bn, bd
        integer(c_int64_t), intent(out) :: cn, cd
        cn = an * bd + bn * ad
        cd = ad * bd
        call rat_reduce(cn, cd)
    end subroutine

    subroutine rat_sub(an, ad, bn, bd, cn, cd) bind(C, name="rat_sub")
        integer(c_int64_t), intent(in), value :: an, ad, bn, bd
        integer(c_int64_t), intent(out) :: cn, cd
        cn = an * bd - bn * ad
        cd = ad * bd
        call rat_reduce(cn, cd)
    end subroutine

    subroutine rat_mul(an, ad, bn, bd, cn, cd) bind(C, name="rat_mul")
        integer(c_int64_t), intent(in), value :: an, ad, bn, bd
        integer(c_int64_t), intent(out) :: cn, cd
        cn = an * bn
        cd = ad * bd
        call rat_reduce(cn, cd)
    end subroutine

    subroutine rat_div(an, ad, bn, bd, cn, cd) bind(C, name="rat_div")
        integer(c_int64_t), intent(in), value :: an, ad, bn, bd
        integer(c_int64_t), intent(out) :: cn, cd
        cn = an * bd
        cd = ad * bn
        call rat_reduce(cn, cd)
    end subroutine

end module rational_ops
