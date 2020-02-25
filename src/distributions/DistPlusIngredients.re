open DistTypes;

let make =
    (~guesstimatorString, ~domain=Complete, ~unit=UnspecifiedDistribution, ())
    : distPlusIngredients => {
  guesstimatorString,
  domain,
  unit,
};

let toDistPlus =
    (
      ~sampleCount=2000,
      ~outputXYPoints=2000,
      ~truncateTo=Some(100),
      t: distPlusIngredients,
    )
    : option(distPlus) => {
  let shape =
    Guesstimator.stringToMixedShape(
      ~string=t.guesstimatorString,
      ~sampleCount,
      ~outputXYPoints,
      ~truncateTo,
      (),
    );
  let distPlus =
    shape
    |> E.O.fmap(
         Distributions.DistPlus.make(
           ~shape=_,
           ~domain=t.domain,
           ~unit=t.unit,
           ~guesstimatorString=None,
           (),
         ),
       )
    |> E.O.fmap(
         Distributions.DistPlus.T.scaleToIntegralSum(~intendedSum=1.0),
       );
  distPlus;
};