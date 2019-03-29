do
	(set i (int 1))
	(set n (int 1000))
	(while
		(lt (get i) (get n))
		(do
			(print (get i))
			(set i (add (get i) (int 1)))
		)
	)
	(print Done)
