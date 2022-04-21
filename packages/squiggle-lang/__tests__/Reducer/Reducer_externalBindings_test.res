open Jest
open Reducer_TestHelpers

describe("Parse for Bindings", () => {
  testParseOuterToBe(
    "x",
    "Ok((:$$bindExpression (:$$bindings) :x))",
    ) 
  testParseOuterToBe(
    "x+1",
    "Ok((:$$bindExpression (:$$bindings) (:add :x 1)))",
    ) 
  testParseOuterToBe(
    "y = x+1; y",
    "Ok((:$$bindExpression (:$$bindStatement (:$$bindings) (:$let :y (:add :x 1))) :y))",
    ) 
  testParsePartialToBe(
    "x",
    "Ok((:$$bindExpression (:$$bindStatement (:$$bindings) :x) (:$exportVariablesExpression)))",
    ) 
  testParsePartialToBe(
    "y=x",
    "Ok((:$$bindExpression (:$$bindStatement (:$$bindings) (:$let :y :x)) (:$exportVariablesExpression)))",
    ) 
  testParsePartialToBe(
    "y=x+1",
    "Ok((:$$bindExpression (:$$bindStatement (:$$bindings) (:$let :y (:add :x 1))) (:$exportVariablesExpression)))",
    ) 
  testParsePartialToBe(
    "y = x+1; z = y",
    "Ok((:$$bindExpression (:$$bindStatement (:$$bindStatement (:$$bindings) (:$let :y (:add :x 1))) (:$let :z :y)) (:$exportVariablesExpression)))",
    ) 
})

describe("Eval with Bindings", () => {
  testEvalBindingsToBe(
    "x",
    list{("x", ExpressionValue.EvNumber(1.))},
    "Ok(1)",
    ) 
  testEvalBindingsToBe(
    "x+1",
    list{("x", ExpressionValue.EvNumber(1.))},
    "Ok(2)",
    ) 
  testEvalBindingsToBe(
    "y = x+1; y",
    list{("x", ExpressionValue.EvNumber(1.))},
    "Ok(2)",
    ) 
})

Only.describe("Eval Partial", () => {
  MyOnly.testEvalPartialBindingsToBe(
    //   A partial cannot end with an expression
    "x",
    list{("x", ExpressionValue.EvNumber(1.))},
    "Error(Assignment expected)",
    ) 
  testEvalPartialBindingsToBe(
    "y=x",
    list{("x", ExpressionValue.EvNumber(1.))},
    "????",
    ) 
  testEvalBindingsToBe(
    "y=x+1",
    list{("x", ExpressionValue.EvNumber(1.))},
    "????",
    ) 
  testEvalBindingsToBe(
    "y = x+1; z = y",
    list{("x", ExpressionValue.EvNumber(1.))},
    "????",
    ) 
})

