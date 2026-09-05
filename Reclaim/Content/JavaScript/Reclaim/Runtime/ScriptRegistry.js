"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.registerScriptController = registerScriptController;
exports.createScriptController = createScriptController;
exports.hasScriptController = hasScriptController;
const factories = new Map();
function registerScriptController(scriptId, factory) {
    if (!scriptId) {
        throw new Error("registerScriptController requires a non-empty scriptId");
    }
    if (factories.has(scriptId)) {
        throw new Error(`Duplicate Reclaim script controller: ${scriptId}`);
    }
    factories.set(scriptId, factory);
}
function createScriptController(scriptId, binding) {
    const factory = factories.get(scriptId);
    return factory ? factory(binding) : null;
}
function hasScriptController(scriptId) {
    return factories.has(scriptId);
}
