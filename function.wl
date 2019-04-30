do
	(set fibo (func
		(a)
		(if
            (lt (get a) (int 3))
            (int 1)
            (add
                (call fibo (sub (get a) (int 1)))
                (call fibo (sub (get a) (int 2)))
            )
        )
	))
    (set facto (func
        (a)
        (if
            (eq (get a) (int 1))
            (int 1)
            (mul
                (get a)
                (call facto (sub (get a) (int 1)))
            )
        )
    ))
	(print (call fibo (int 40)))
