// initialize each position in the array to the value of the previous position + 1, starting with value v for the first element
// similar to https://github.com/sosy-lab/sv-benchmarks/blob/master/c/array-examples/standard_seq_init_true-unreach-call_ground.c

func main()
{
	Int[] a;
	const Int alength;
	const Int v;

	Int i = 0;
  a[0] = v;
	while((i+1) < alength)
	{
		a[i + 1] = a[i] + 1;
		i = i + 1;
	}
}

(conjecture
   (forall ((pos Int))
      (=>
         (and
            (<= 0 pos)
            (< (+ pos 1) (value_const_int alength))
            (<= 0 (value_const_int alength))
         )
         (<= (select (value_arr main_end a) pos) (select (value_arr main_end a) (+ pos 1)))
      )
   )
)

(conjecture
   (forall ((pos Int))
      (=>
         (and
            (<= 0 pos)
            (< (+ pos 1) (value_const_int alength))
            (<= 0 (value_const_int alength))
         )
         (< (select (value_arr main_end a) pos) (select (value_arr main_end a) (+ pos 1)))
      )
   )
)
