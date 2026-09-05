"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.ReclaimScriptController = void 0;
class ReclaimScriptController {
    constructor(binding) {
        this.binding = binding;
    }
    onAttach() { }
    onEvent(_event) { }
    onDetach() { }
    emitCommand(commandName, context = null, payload = {}) {
        const payloadJson = typeof payload === "string" ? payload : JSON.stringify(payload !== null && payload !== void 0 ? payload : {});
        this.binding.EmitScriptCommand(commandName, context, payloadJson);
    }
    parsePayload(payloadJson, fallback) {
        if (!payloadJson) {
            return fallback;
        }
        try {
            return JSON.parse(payloadJson);
        }
        catch {
            return fallback;
        }
    }
}
exports.ReclaimScriptController = ReclaimScriptController;
