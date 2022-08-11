open FunctionRegistry_Core
open FunctionRegistry_Helpers

let nameSpace = "Pointset"
let requiresNamespace = true

let inputsTodist = (inputs: array<FunctionRegistry_Core.frValue>, makeDist) => {
  let array = inputs->getOrError(0)->E.R.bind(Prepare.ToValueArray.Array.openA)
  let xyCoords =
    array->E.R.bind(xyCoords =>
      xyCoords
      ->E.A2.fmap(xyCoord =>
        [xyCoord]->Prepare.ToValueArray.Record.twoArgs->E.R.bind(Prepare.ToValueTuple.twoNumbers)
      )
      ->E.A.R.firstErrorOrOpen
    )
  let expressionValue =
    xyCoords
    ->E.R.bind(r => r->XYShape.T.makeFromZipped->E.R2.errMap(XYShape.Error.toString))
    ->E.R2.fmap(r => ReducerInterface_InternalExpressionValue.IEvDistribution(
      PointSet(makeDist(r)),
    ))
  expressionValue
}

let library = [
  Function.make(
    ~name="makeContinuous",
    ~nameSpace,
    ~requiresNamespace,
    ~examples=[
      `Pointset.makeContinuous([
        {x: 0, y: 0.2},
        {x: 1, y: 0.7},
        {x: 2, y: 0.8},
        {x: 3, y: 0.2}
      ])`,
    ],
    ~output=ReducerInterface_InternalExpressionValue.EvtDistribution,
    ~definitions=[
      FnDefinition.make(
        ~name="makeContinuous",
        ~inputs=[FRTypeArray(FRTypeRecord([("x", FRTypeNumeric), ("y", FRTypeNumeric)]))],
        ~run=(_, inputs, _) => inputsTodist(inputs, r => Continuous(Continuous.make(r))),
        (),
      ),
    ],
    (),
  ),
  Function.make(
    ~name="makeDiscrete",
    ~nameSpace,
    ~requiresNamespace,
    ~examples=[
      `Pointset.makeDiscrete([
        {x: 0, y: 0.2},
        {x: 1, y: 0.7},
        {x: 2, y: 0.8},
        {x: 3, y: 0.2}
      ])`,
    ],
    ~output=ReducerInterface_InternalExpressionValue.EvtDistribution,
    ~definitions=[
      FnDefinition.make(
        ~name="makeDiscrete",
        ~inputs=[FRTypeArray(FRTypeRecord([("x", FRTypeNumeric), ("y", FRTypeNumeric)]))],
        ~run=(_, inputs, _) => inputsTodist(inputs, r => Discrete(Discrete.make(r))),
        (),
      ),
    ],
    (),
  ),
]
