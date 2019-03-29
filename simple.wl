do
	(set i (int 1))
	(set n (int 10))
	(while
		(ne (get i) (int 15))
		(do
			(print (get i))
			(set i (add (get i) (int 1)))
		)
	)
	(print Done)
