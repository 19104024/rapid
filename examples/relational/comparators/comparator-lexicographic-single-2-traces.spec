(set-traces 2)

func main()
{
	const Int a;
	const Int b;

	// return value: 20 encodes a<b, 10 encodes a=b, 30 encodes b<a
	Int return;

	if (a != b)
	{
		if (a < b)
		{
			return = 20;
		}
		else
		{
			return = 30;
		}
	}
	else
	{
		return = 10;
	}
}

(conjecture
	(=>
		(and
			(= (a t1) (b t2))
			(= (b t1) (a t2))

			(= (return main_end t1) 10)
		)
		(= (return main_end t2) 10)
	)
)

(conjecture
	(=>
		(and
			(= (a t1) (b t2))
			(= (b t1) (a t2))

			(= (return main_end t1) 20)
		)
		(= (return main_end t2) 30)
	)
)