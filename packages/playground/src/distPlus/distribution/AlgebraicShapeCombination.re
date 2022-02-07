type pointMassesWithMoments = {
  n: int,
  masses: array(float),
  means: array(float),
  variances: array(float),
};

/* This function takes a continuous distribution and efficiently approximates it as
   point masses that have variances associated with them.
   We estimate the means and variances from overlapping triangular distributions which we imagine are making up the
   XYShape.
   We can then use the algebra of random variables to "convolve" the point masses and their variances,
   and finally reconstruct a new distribution from them, e.g. using a Fast Gauss Transform or Raykar et al. (2007). */
let toDiscretePointMassesFromTriangulars =
    (~inverse=false, s: XYShape.T.t): pointMassesWithMoments => {
  // TODO: what if there is only one point in the distribution?
  let n = s |> XYShape.T.length;
  // first, double up the leftmost and rightmost points:
  let {xs, ys}: XYShape.T.t = s;
  Js.Array.unshift(xs[0], xs) |> ignore;
  Js.Array.unshift(ys[0], ys) |> ignore;
  Js.Array.push(xs[n - 1], xs) |> ignore;
  Js.Array.push(ys[n - 1], ys) |> ignore;
  let n = E.A.length(xs);
  // squares and neighbourly products of the xs
  let xsSq: array(float) = Belt.Array.makeUninitializedUnsafe(n);
  let xsProdN1: array(float) = Belt.Array.makeUninitializedUnsafe(n - 1);
  let xsProdN2: array(float) = Belt.Array.makeUninitializedUnsafe(n - 2);
  for (i in 0 to n - 1) {
    Belt.Array.set(xsSq, i, xs[i] *. xs[i]) |> ignore;
    ();
  };
  for (i in 0 to n - 2) {
    Belt.Array.set(xsProdN1, i, xs[i] *. xs[i + 1]) |> ignore;
    ();
  };
  for (i in 0 to n - 3) {
    Belt.Array.set(xsProdN2, i, xs[i] *. xs[i + 2]) |> ignore;
    ();
  };
  // means and variances
  let masses: array(float) = Belt.Array.makeUninitializedUnsafe(n - 2); // doesn't include the fake first and last points
  let means: array(float) = Belt.Array.makeUninitializedUnsafe(n - 2);
  let variances: array(float) = Belt.Array.makeUninitializedUnsafe(n - 2);

  if (inverse) {
    for (i in 1 to n - 2) {
      Belt.Array.set(masses, i - 1, (xs[i + 1] -. xs[i - 1]) *. ys[i] /. 2.)
      |> ignore;

      // this only works when the whole triange is either on the left or on the right of zero
      let a = xs[i - 1];
      let c = xs[i];
      let b = xs[i + 1];

      // These are the moments of the reciprocal of a triangular distribution, as symbolically integrated by Mathematica.
      // They're probably pretty close to invMean ~ 1/mean = 3/(a+b+c) and invVar. But I haven't worked out
      // the worst case error, so for now let's use these monster equations
      let inverseMean =
        2.
        *. (a *. log(a /. c) /. (a -. c) +. b *. log(c /. b) /. (b -. c))
        /. (a -. b);
      let inverseVar =
        2.
        *. (log(c /. a) /. (a -. c) +. b *. log(b /. c) /. (b -. c))
        /. (a -. b)
        -. inverseMean
        ** 2.;

      Belt.Array.set(means, i - 1, inverseMean) |> ignore;

      Belt.Array.set(variances, i - 1, inverseVar) |> ignore;
      ();
    };

    {n: n - 2, masses, means, variances};
  } else {
    for (i in 1 to n - 2) {
      // area of triangle = width * height / 2
      Belt.Array.set(masses, i - 1, (xs[i + 1] -. xs[i - 1]) *. ys[i] /. 2.)
      |> ignore;

      // means of triangle = (a + b + c) / 3
      Belt.Array.set(means, i - 1, (xs[i - 1] +. xs[i] +. xs[i + 1]) /. 3.)
      |> ignore;

      // variance of triangle = (a^2 + b^2 + c^2 - ab - ac - bc) / 18
      Belt.Array.set(
        variances,
        i - 1,
        (
          xsSq[i - 1]
          +. xsSq[i]
          +. xsSq[i + 1]
          -. xsProdN1[i - 1]
          -. xsProdN1[i]
          -. xsProdN2[i - 1]
        )
        /. 18.,
      )
      |> ignore;
      ();
    };
    {n: n - 2, masses, means, variances};
  };
};

let combineShapesContinuousContinuous =
    (
      op: ExpressionTypes.algebraicOperation,
      s1: DistTypes.xyShape,
      s2: DistTypes.xyShape,
    )
    : DistTypes.xyShape => {
  let t1n = s1 |> XYShape.T.length;
  let t2n = s2 |> XYShape.T.length;

  // if we add the two distributions, we should probably use normal filters.
  // if we multiply the two distributions, we should probably use lognormal filters.
  let t1m = toDiscretePointMassesFromTriangulars(s1);
  let t2m =
    switch (op) {
    | `Divide => toDiscretePointMassesFromTriangulars(~inverse=true, s2)
    | _ => toDiscretePointMassesFromTriangulars(~inverse=false, s2)
    };

  let combineMeansFn =
    switch (op) {
    | `Add => ((m1, m2) => m1 +. m2)
    | `Subtract => ((m1, m2) => m1 -. m2)
    | `Multiply => ((m1, m2) => m1 *. m2)
    | `Divide => ((m1, mInv2) => m1 *. mInv2)
    | `Exponentiate => ((m1, mInv2) => m1 ** mInv2)
    }; // note: here, mInv2 = mean(1 / t2) ~= 1 / mean(t2)

  // TODO: I don't know what the variances are for exponentatiation
  // converts the variances and means of the two inputs into the variance of the output
  let combineVariancesFn =
    switch (op) {
    | `Add => ((v1, v2, _, _) => v1 +. v2)
    | `Subtract => ((v1, v2, _, _) => v1 +. v2)
    | `Multiply => (
        (v1, v2, m1, m2) => v1 *. v2 +. v1 *. m2 ** 2. +. v2 *. m1 ** 2.
      )
    | `Exponentiate =>
      ((v1, v2, m1, m2) => v1 *. v2 +. v1 *. m2 ** 2. +. v2 *. m1 ** 2.);
    | `Divide => (
        (v1, vInv2, m1, mInv2) =>
          v1 *. vInv2 +. v1 *. mInv2 ** 2. +. vInv2 *. m1 ** 2.
      )
    };

  // TODO: If operating on two positive-domain distributions, we should take that into account
  let outputMinX: ref(float) = ref(infinity);
  let outputMaxX: ref(float) = ref(neg_infinity);
  let masses: array(float) =
    Belt.Array.makeUninitializedUnsafe(t1m.n * t2m.n);
  let means: array(float) =
    Belt.Array.makeUninitializedUnsafe(t1m.n * t2m.n);
  let variances: array(float) =
    Belt.Array.makeUninitializedUnsafe(t1m.n * t2m.n);
  // then convolve the two sets of pointMassesWithMoments
  for (i in 0 to t1m.n - 1) {
    for (j in 0 to t2m.n - 1) {
      let k = i * t2m.n + j;
      Belt.Array.set(masses, k, t1m.masses[i] *. t2m.masses[j]) |> ignore;

      let mean = combineMeansFn(t1m.means[i], t2m.means[j]);
      let variance =
        combineVariancesFn(
          t1m.variances[i],
          t2m.variances[j],
          t1m.means[i],
          t2m.means[j],
        );
      Belt.Array.set(means, k, mean) |> ignore;
      Belt.Array.set(variances, k, variance) |> ignore;
      // update bounds
      let minX = mean -. 2. *. sqrt(variance) *. 1.644854;
      let maxX = mean +. 2. *. sqrt(variance) *. 1.644854;
      if (minX < outputMinX^) {
        outputMinX := minX;
      };
      if (maxX > outputMaxX^) {
        outputMaxX := maxX;
      };
    };
  };

  // we now want to create a set of target points. For now, let's just evenly distribute 200 points between
  // between the outputMinX and outputMaxX
  let nOut = 300;
  let outputXs: array(float) =
    E.A.Floats.range(outputMinX^, outputMaxX^, nOut);
  let outputYs: array(float) = Belt.Array.make(nOut, 0.0);
  // now, for each of the outputYs, accumulate from a Gaussian kernel over each input point.
  for (j in 0 to E.A.length(masses) - 1) {
    // go through all of the result points
    if (variances[j] > 0. && masses[j] > 0.) {
      for (i in 0 to E.A.length(outputXs) - 1) {
        // go through all of the target points
        let dx = outputXs[i] -. means[j];
        let contribution =
          masses[j]
          *. exp(-. (dx ** 2.) /. (2. *. variances[j]))
          /. sqrt(2. *. 3.14159276 *. variances[j]);
        Belt.Array.set(outputYs, i, outputYs[i] +. contribution) |> ignore;
      };
    };
  };

  {xs: outputXs, ys: outputYs};
};

let toDiscretePointMassesFromDiscrete =
    (s: DistTypes.xyShape): pointMassesWithMoments => {
  let {xs, ys}: XYShape.T.t = s;
  let n = E.A.length(xs);

  let masses: array(float) = Belt.Array.makeBy(n, i => ys[i]);
  let means: array(float) = Belt.Array.makeBy(n, i => xs[i]);
  let variances: array(float) = Belt.Array.makeBy(n, i => 0.0);

  {n, masses, means, variances};
};

let combineShapesContinuousDiscrete =
    (
      op: ExpressionTypes.algebraicOperation,
      continuousShape: DistTypes.xyShape,
      discreteShape: DistTypes.xyShape,
    )
    : DistTypes.xyShape => {
  let t1n = continuousShape |> XYShape.T.length;
  let t2n = discreteShape |> XYShape.T.length;

  // each x pair is added/subtracted
  let fn = Operation.Algebraic.toFn(op);

  let outXYShapes: array(array((float, float))) =
    Belt.Array.makeUninitializedUnsafe(t2n);

  switch (op) {
  | `Add
  | `Subtract =>
    for (j in 0 to t2n - 1) {
      // creates a new continuous shape for each one of the discrete points, and collects them in outXYShapes.
      let dxyShape: array((float, float)) =
        Belt.Array.makeUninitializedUnsafe(t1n);
      for (i in 0 to t1n - 1) {
        Belt.Array.set(
          dxyShape,
          i,
          (
            fn(continuousShape.xs[i], discreteShape.xs[j]),
            continuousShape.ys[i] *. discreteShape.ys[j],
          ),
        )
        |> ignore;
        ();
      };
      Belt.Array.set(outXYShapes, j, dxyShape) |> ignore;
      ();
    }
  | `Multiply
  | `Exponentiate
  | `Divide =>
    for (j in 0 to t2n - 1) {
      // creates a new continuous shape for each one of the discrete points, and collects them in outXYShapes.
      let dxyShape: array((float, float)) =
        Belt.Array.makeUninitializedUnsafe(t1n);
      for (i in 0 to t1n - 1) {
        Belt.Array.set(
          dxyShape,
          i,
          (
            fn(continuousShape.xs[i], discreteShape.xs[j]),
            {continuousShape.ys[i] *. discreteShape.ys[j] /. discreteShape.xs[j]}
          ),
        )
        |> ignore;
        ();
      };
      Belt.Array.set(outXYShapes, j, dxyShape) |> ignore;
      ();
    }
  };

  outXYShapes
  |> E.A.fmap(XYShape.T.fromZippedArray)
  |> E.A.fold_left(
       XYShape.PointwiseCombination.combine(
         (+.),
         XYShape.XtoY.continuousInterpolator(`Linear, `UseZero),
       ),
       XYShape.T.empty,
     );
};
