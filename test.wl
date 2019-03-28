do
	(set fna (func
		(x)
		(print Done)
	))
	(set i (int 1))
	(set n (int 10))
	(while
		(lt (get i) (get n))
		(do
			(print (get i))
			(set i (add (get i) (int 1)))
		)
	)
	(print (get fna))
	(call fna 1)
