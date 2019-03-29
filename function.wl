do
	(set fna (func
		(a)
		(if
            (lt (get a) (int 2))
            (int 1)
            (add
                (call fna (sub (get a) (int 1)))
                (call fna (sub (get a) (int 2)))
            )
        )
	))
	(print (call fna (int 2)))
