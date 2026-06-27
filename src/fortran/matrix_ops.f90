! SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
! SPDX-License-Identifier: GPL-3.0-only

! All arrays are passed as 1D with explicit dimensions to keep row-major (C)
! order consistent without column-major confusion.

module matrix_ops
    use iso_c_binding
    implicit none
contains

    ! C = A * B  where A is m x n, B is n x k, C is m x k (row-major)
    subroutine mat_mul(a, b, c, m, n, k) bind(C, name="mat_mul")
        integer(c_int32_t), intent(in), value :: m, n, k
        real(c_double), intent(in)  :: a(m*n), b(n*k)
        real(c_double), intent(out) :: c(m*k)
        integer :: i, j, p
        real(c_double) :: s
        do i = 1, m
            do j = 1, k
                s = 0.0d0
                do p = 1, n
                    s = s + a((i-1)*n + p) * b((p-1)*k + j)
                end do
                c((i-1)*k + j) = s
            end do
        end do
    end subroutine

    ! Transpose: B = A^T, A is m x n, B is n x m (row-major)
    subroutine mat_transpose(a, b, m, n) bind(C, name="mat_transpose")
        integer(c_int32_t), intent(in), value :: m, n
        real(c_double), intent(in)  :: a(m*n)
        real(c_double), intent(out) :: b(n*m)
        integer :: i, j
        do i = 1, m
            do j = 1, n
                b((j-1)*m + i) = a((i-1)*n + j)
            end do
        end do
    end subroutine

    ! Determinant of n x n matrix via Gaussian elimination with partial pivoting
    function mat_det(a, n) result(d) bind(C, name="mat_det")
        integer(c_int32_t), intent(in), value :: n
        real(c_double), intent(in) :: a(n*n)
        real(c_double) :: d
        real(c_double) :: lu(n*n)
        real(c_double) :: factor, pivot, tmp
        integer :: i, j, k, pivot_row, sign_flip
        integer :: pi, ki, ij

        lu = a
        sign_flip = 1
        d = 1.0d0

        do k = 1, n
            ! find pivot in column k
            pivot = abs(lu((k-1)*n + k))
            pivot_row = k
            do i = k+1, n
                tmp = abs(lu((i-1)*n + k))
                if (tmp > pivot) then
                    pivot = tmp
                    pivot_row = i
                end if
            end do

            if (pivot < 1.0d-15) then
                d = 0.0d0
                return
            end if

            ! swap rows k and pivot_row
            if (pivot_row /= k) then
                do j = 1, n
                    pi = (pivot_row-1)*n + j
                    ki = (k-1)*n + j
                    tmp = lu(pi)
                    lu(pi) = lu(ki)
                    lu(ki) = tmp
                end do
                sign_flip = -sign_flip
            end if

            ! eliminate column k below pivot
            do i = k+1, n
                factor = lu((i-1)*n + k) / lu((k-1)*n + k)
                do j = k, n
                    ij = (i-1)*n + j
                    lu(ij) = lu(ij) - factor * lu((k-1)*n + j)
                end do
            end do
        end do

        ! product of diagonal
        do k = 1, n
            d = d * lu((k-1)*n + k)
        end do
        d = d * sign_flip
    end function

    ! Rank of m x n matrix via Gaussian elimination
    function mat_rank(a, m, n) result(r) bind(C, name="mat_rank")
        integer(c_int32_t), intent(in), value :: m, n
        real(c_double), intent(in) :: a(m*n)
        integer(c_int32_t) :: r
        real(c_double) :: work(m*n)
        real(c_double) :: factor, pivot, tmp
        integer :: i, j, pivot_row, col
        integer :: pi, ki, ij
        integer :: row_ptr

        work = a
        row_ptr = 1
        r = 0

        do col = 1, n
            if (row_ptr > m) exit
            ! find pivot in this column from row_ptr downward
            pivot = 0.0d0
            pivot_row = 0
            do i = row_ptr, m
                tmp = abs(work((i-1)*n + col))
                if (tmp > pivot) then
                    pivot = tmp
                    pivot_row = i
                end if
            end do
            if (pivot < 1.0d-12) cycle

            ! swap rows row_ptr and pivot_row
            if (pivot_row /= row_ptr) then
                do j = 1, n
                    pi = (pivot_row-1)*n + j
                    ki = (row_ptr-1)*n + j
                    tmp = work(pi)
                    work(pi) = work(ki)
                    work(ki) = tmp
                end do
            end if

            ! eliminate below
            do i = row_ptr+1, m
                factor = work((i-1)*n + col) / work((row_ptr-1)*n + col)
                do j = col, n
                    ij = (i-1)*n + j
                    work(ij) = work(ij) - factor * work((row_ptr-1)*n + j)
                end do
            end do

            r = r + 1
            row_ptr = row_ptr + 1
        end do
    end function

end module matrix_ops
