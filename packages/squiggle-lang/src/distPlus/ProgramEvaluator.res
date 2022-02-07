// TODO: This setup is more confusing than it should be, there's more work to do in cleanup here.
module Inputs = {
  module SamplingInputs = {
    type t = {
      sampleCount: option<int>,
      outputXYPoints: option<int>,
      kernelWidth: option<float>,
      shapeLength: option<int>,
    }
  }
  let defaultRecommendedLength = 100
  let defaultShouldDownsample = true

  type inputs = {
    squiggleString: string,
    samplingInputs: SamplingInputs.t,
    environment: ExpressionTypes.ExpressionTree.environment,
  }

  let empty: SamplingInputs.t = {
    sampleCount: None,
    outputXYPoints: None,
    kernelWidth: None,
    shapeLength: None,
  }

  let make = (
    ~samplingInputs=empty,
    ~squiggleString,
    ~environment=ExpressionTypes.ExpressionTree.Environment.empty,
    (),
  ): inputs => {
    samplingInputs: samplingInputs,
    squiggleString: squiggleString,
    environment: environment,
  }
}

type \"export" = [
  | #DistPlus(SquiggleExperimental.DistPlus.t)
  | #Float(float)
  | #Function(
    (array<string>, SquiggleExperimental.ExpressionTypes.ExpressionTree.node),
    SquiggleExperimental.ExpressionTypes.ExpressionTree.environment,
  )
]

module Internals = {
  let addVariable = (
    {samplingInputs, squiggleString, environment}: Inputs.inputs,
    str,
    node,
  ): Inputs.inputs => {
    samplingInputs: samplingInputs,
    squiggleString: squiggleString,
    environment: ExpressionTypes.ExpressionTree.Environment.update(environment, str, _ => Some(
      node,
    )),
  }

  type outputs = {
    graph: ExpressionTypes.ExpressionTree.node,
    shape: DistTypes.shape,
  }
  let makeOutputs = (graph, shape): outputs => {graph: graph, shape: shape}

  let makeInputs = (inputs: Inputs.inputs): ExpressionTypes.ExpressionTree.samplingInputs => {
    sampleCount: inputs.samplingInputs.sampleCount |> E.O.default(10000),
    outputXYPoints: inputs.samplingInputs.outputXYPoints |> E.O.default(10000),
    kernelWidth: inputs.samplingInputs.kernelWidth,
    shapeLength: inputs.samplingInputs.shapeLength |> E.O.default(10000),
  }

  let runNode = (inputs, node) =>
    ExpressionTree.toLeaf(makeInputs(inputs), inputs.environment, node)

  let runProgram = (inputs: Inputs.inputs, p: ExpressionTypes.Program.program) => {
    let ins = ref(inputs)
    p
    |> E.A.fmap(x =>
      switch x {
      | #Assignment(name, node) =>
        ins := addVariable(ins.contents, name, node)
        None
      | #Expression(node) =>
        Some(runNode(ins.contents, node) |> E.R.fmap(r => (ins.contents.environment, r)))
      }
    )
    |> E.A.O.concatSomes
    |> E.A.R.firstErrorOrOpen
  }

  let inputsToLeaf = (inputs: Inputs.inputs) =>
    MathJsParser.fromString(inputs.squiggleString) |> E.R.bind(_, g => runProgram(inputs, g))

  let outputToDistPlus = (inputs: Inputs.inputs, shape: DistTypes.shape) =>
    DistPlus.make(~shape, ~squiggleString=Some(inputs.squiggleString), ())
}

let renderIfNeeded = (inputs: Inputs.inputs, node: ExpressionTypes.ExpressionTree.node): result<
  ExpressionTypes.ExpressionTree.node,
  string,
> =>
  node |> (
    x =>
      switch x {
      | #Normalize(_) as n
      | #SymbolicDist(_) as n =>
        #Render(n)
        |> Internals.runNode(inputs)
        |> (
          x =>
            switch x {
            | Ok(#RenderedDist(_)) as r => r
            | Error(r) => Error(r)
            | _ => Error("Didn't render, but intended to")
            }
        )

      | n => Ok(n)
      }
  )

// TODO: Consider using ExpressionTypes.ExpressionTree.getFloat or similar in this function
let coersionToExportedTypes = (
  inputs,
  env: SquiggleExperimental.ExpressionTypes.ExpressionTree.environment,
  node: ExpressionTypes.ExpressionTree.node,
): result<\"export", string> =>
  node
  |> renderIfNeeded(inputs)
  |> E.R.bind(_, x =>
    switch x {
    | #RenderedDist(Discrete({xyShape: {xs: [x], ys: [1.0]}})) => Ok(#Float(x))
    | #SymbolicDist(#Float(x)) => Ok(#Float(x))
    | #RenderedDist(n) => Ok(#DistPlus(Internals.outputToDistPlus(inputs, n)))
    | #Function(n) => Ok(#Function(n, env))
    | n => Error("Didn't output a rendered distribution. Format:" ++ ExpressionTree.toString(n))
    }
  )

let rec mapM = (f, xs) =>
  switch xs {
  | list{} => Ok(list{})
  | list{x, ...rest} =>
    switch f(x) {
    | Error(err) => Error(err)
    | Ok(val) =>
      switch mapM(f, rest) {
      | Error(err) => Error(err)
      | Ok(restList) => Ok(list{val, ...restList})
      }
    }
  }

let evaluateProgram = (inputs: Inputs.inputs) =>
  inputs
  |> Internals.inputsToLeaf
  |> E.R.bind(_, xs => mapM(((a, b)) => coersionToExportedTypes(inputs, a, b), Array.to_list(xs)))

let evaluateFunction = (
  inputs: Inputs.inputs,
  fn: (array<string>, ExpressionTypes.ExpressionTree.node),
  fnInputs,
) => {
  let output = ExpressionTree.runFunction(
    Internals.makeInputs(inputs),
    inputs.environment,
    fnInputs,
    fn,
  )
  output |> E.R.bind(_, coersionToExportedTypes(inputs, inputs.environment))
}

@genType
let runAll = (squiggleString: string) => {
  let inputs = Inputs.make(
    ~samplingInputs={
      sampleCount: Some(10000),
      outputXYPoints: Some(10000),
      kernelWidth: None,
      shapeLength: Some(1000),
    },
    ~squiggleString,
    ~environment=[]->Belt.Map.String.fromArray,
    (),
  )
  let response1 = evaluateProgram(inputs);
  response1;
}
