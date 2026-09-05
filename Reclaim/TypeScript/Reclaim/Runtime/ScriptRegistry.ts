import { ScriptController, ScriptControllerFactory } from "./ScriptController";

const factories = new Map<string, ScriptControllerFactory>();

export function registerScriptController(scriptId: string, factory: ScriptControllerFactory): void {
    if (!scriptId) {
        throw new Error("registerScriptController requires a non-empty scriptId");
    }
    if (factories.has(scriptId)) {
        throw new Error(`Duplicate Reclaim script controller: ${scriptId}`);
    }
    factories.set(scriptId, factory);
}

export function createScriptController(scriptId: string, binding: any): ScriptController | null {
    const factory = factories.get(scriptId);
    return factory ? factory(binding) : null;
}

export function hasScriptController(scriptId: string): boolean {
    return factories.has(scriptId);
}
