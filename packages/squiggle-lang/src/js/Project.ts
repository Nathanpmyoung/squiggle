import * as RSProject from "../rescript/ForTS/ForTS_ReducerProject.gen";
import { reducerErrorValue } from "../rescript/ForTS/ForTS_Reducer_ErrorValue.gen";
import { environment } from "../rescript/ForTS/ForTS_Distribution/ForTS_Distribution_Environment.gen";
import { ErrorValue } from "./ErrorValue";
import { NameSpace as NameSpace } from "./NameSpace";
import { wrapSquiggleValue } from "./SquiggleValue";
import { resultMap2 } from "./types";

export class Project {
  _value: RSProject.reducerProject;

  constructor(_value: RSProject.reducerProject) {
    this._value = _value;
  }

  static create() {
    return new Project(RSProject.createProject());
  }

  getSourceIds() {
    return RSProject.getSourceIds(this._value);
  }

  setSource(sourceId: string, value: string) {
    return RSProject.setSource(this._value, sourceId, value);
  }

  getSource(sourceId: string) {
    return RSProject.getSource(this._value, sourceId);
  }

  touchSource(sourceId: string) {
    return RSProject.touchSource(this._value, sourceId);
  }

  clean(sourceId: string) {
    return RSProject.clean(this._value, sourceId);
  }

  cleanAll() {
    return RSProject.cleanAll(this._value);
  }

  cleanResults(sourceId: string) {
    return RSProject.cleanResults(this._value, sourceId);
  }

  cleanAllResults() {
    return RSProject.cleanAllResults(this._value);
  }

  getIncludes(sourceId: string) {
    return resultMap2(
      RSProject.getIncludes(this._value, sourceId),
      (a) => a,
      (v: reducerErrorValue) => new ErrorValue(v)
    );
  }

  getContinues(sourceId: string) {
    return RSProject.getContinues(this._value, sourceId);
  }

  setContinues(sourceId: string, continues: string[]) {
    return RSProject.setContinues(this._value, sourceId, continues);
  }

  getDependencies(sourceId: string) {
    return RSProject.getDependencies(this._value, sourceId);
  }

  getDependents(sourceId: string) {
    return RSProject.getDependents(this._value, sourceId);
  }

  getRunOrder() {
    return RSProject.getRunOrder(this._value);
  }

  getRunOrderFor(sourceId: string) {
    return RSProject.getRunOrderFor(this._value, sourceId);
  }

  parseIncludes(sourceId: string) {
    return RSProject.parseIncludes(this._value, sourceId);
  }

  run(sourceId: string) {
    return RSProject.run(this._value, sourceId);
  }

  runAll() {
    return RSProject.runAll(this._value);
  }

  getBindings(sourceId: string) {
    return new NameSpace(RSProject.getBindings(this._value, sourceId));
  }

  getResult(sourceId: string) {
    const innerResult = RSProject.getResult(this._value, sourceId);
    return resultMap2(
      innerResult,
      wrapSquiggleValue,
      (v: reducerErrorValue) => new ErrorValue(v)
    );
  }

  setEnvironment(environment: environment) {
    RSProject.setEnvironment(this._value, environment);
  }
}