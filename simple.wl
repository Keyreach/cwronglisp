do
	(set i (int 1))
	(set n (int 10000))
	(while
		(lt (get i) (get n))
		(do
			(if
                (eq (int 0) (add
                    (mod (get i) (int 3))
                    (mod (get i) (int 5))
                ))
                (print Fizzbuzz)
                (if
                    (eq (int 0) (mod (get i) (int 3)))
                    (print Fizz)
                    (if
                        (eq (int 0) (mod (get i) (int 5)))
                        (print Buzz)
                        (print (get i))
                    )
                )
            )
			(set i (add (get i) (int 1)))
		)
	)
	(print Done)
