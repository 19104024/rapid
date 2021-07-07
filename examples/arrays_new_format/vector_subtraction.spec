// for each position, subtract the element of array b from the element of array a and save the result in array c.
// similar to https://github.com/sosy-lab/sv-benchmarks/blob/master/c/array-examples/standard_seq_init_true-unreach-call_ground.c,
// but subtraction replaced by addition

func main()
{
	const Int[] a;
	const Int[] b;
	Int[] c;
	const Int length;

	Int i = 0;
	while(i < length)
	{
		c[i] = a[i] - b[i];
		i = i + 1;
	}
}

(conjecture
	(forall ((j Int))
		(=>
			(and
				(<= 0 j)
				(< j (value_const_int length))
				(<= 0 (value_const_int length))
			)
			(= 
			    (select (value_arr main_end c) j) 
			    (- (select (value_const_arr a) j) (select (value_const_arr b) j)) 
			)
		)
	)
)

