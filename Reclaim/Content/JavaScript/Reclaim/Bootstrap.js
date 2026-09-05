"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const puerts = require("puerts");
require("./Controllers/index");
const ScriptRegistry_1 = require("./Runtime/ScriptRegistry");
const host = puerts.argv.getByName("ScriptHost");
if (!host) {
    throw new Error("Reclaim Bootstrap: ScriptHost was not provided by C++");
}
const controllers = new Map();
function attach(binding) {
    var _a;
    if (!binding || controllers.has(binding)) {
        return;
    }
    const scriptId = String(binding.GetScriptIdString());
    if (!(0, ScriptRegistry_1.hasScriptController)(scriptId)) {
        console.warn(`[ReclaimTS] no controller registered for '${scriptId}'`);
        return;
    }
    const controller = (0, ScriptRegistry_1.createScriptController)(scriptId, binding);
    if (!controller) {
        return;
    }
    controllers.set(binding, controller);
    (_a = controller.onAttach) === null || _a === void 0 ? void 0 : _a.call(controller);
}
function detach(binding) {
    var _a;
    const controller = controllers.get(binding);
    if (!controller) {
        return;
    }
    (_a = controller.onDetach) === null || _a === void 0 ? void 0 : _a.call(controller);
    controllers.delete(binding);
}
host.OnBindingAdded.Add((binding) => attach(binding));
host.OnBindingRemoved.Add((binding) => detach(binding));
host.OnScriptEvent.Add((binding, eventName, context, payloadJson) => {
    var _a, _b;
    if (!binding) {
        if (eventName === "Runtime.Shutdown") {
            for (const controller of controllers.values()) {
                (_a = controller.onDetach) === null || _a === void 0 ? void 0 : _a.call(controller);
            }
            controllers.clear();
        }
        return;
    }
    const controller = controllers.get(binding);
    (_b = controller === null || controller === void 0 ? void 0 : controller.onEvent) === null || _b === void 0 ? void 0 : _b.call(controller, {
        name: String(eventName),
        context,
        payloadJson: String(payloadJson !== null && payloadJson !== void 0 ? payloadJson : "")
    });
});
const existingBindings = host.GetActiveBindings();
if (existingBindings && typeof existingBindings.Num === "function") {
    const count = existingBindings.Num();
    for (let index = 0; index < count; ++index) {
        attach(existingBindings.Get(index));
    }
}
else if (Array.isArray(existingBindings)) {
    for (const binding of existingBindings) {
        attach(binding);
    }
}
console.warn("[ReclaimTS] runtime started");
