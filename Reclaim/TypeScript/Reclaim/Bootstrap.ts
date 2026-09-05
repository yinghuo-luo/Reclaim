declare function require(name: string): any;

const puerts = require("puerts") as {
    argv: { getByName(name: string): any };
};

import "./Controllers/index";
import { createScriptController, hasScriptController } from "./Runtime/ScriptRegistry";
import { ScriptController } from "./Runtime/ScriptController";

const host = puerts.argv.getByName("ScriptHost");
if (!host) {
    throw new Error("Reclaim Bootstrap: ScriptHost was not provided by C++");
}

const controllers = new Map<any, ScriptController>();

function attach(binding: any): void {
    if (!binding || controllers.has(binding)) {
        return;
    }

    const scriptId = String(binding.GetScriptIdString());
    if (!hasScriptController(scriptId)) {
        console.warn(`[ReclaimTS] no controller registered for '${scriptId}'`);
        return;
    }

    const controller = createScriptController(scriptId, binding);
    if (!controller) {
        return;
    }

    controllers.set(binding, controller);
    controller.onAttach?.();
}

function detach(binding: any): void {
    const controller = controllers.get(binding);
    if (!controller) {
        return;
    }

    controller.onDetach?.();
    controllers.delete(binding);
}

host.OnBindingAdded.Add((binding: any) => attach(binding));
host.OnBindingRemoved.Add((binding: any) => detach(binding));
host.OnScriptEvent.Add((binding: any, eventName: string, context: any, payloadJson: string) => {
    if (!binding) {
        if (eventName === "Runtime.Shutdown") {
            for (const controller of controllers.values()) {
                controller.onDetach?.();
            }
            controllers.clear();
        }
        return;
    }

    const controller = controllers.get(binding);
    controller?.onEvent?.({
        name: String(eventName),
        context,
        payloadJson: String(payloadJson ?? "")
    });
});

const existingBindings = host.GetActiveBindings();
if (existingBindings && typeof existingBindings.Num === "function") {
    const count = existingBindings.Num();
    for (let index = 0; index < count; ++index) {
        attach(existingBindings.Get(index));
    }
} else if (Array.isArray(existingBindings)) {
    for (const binding of existingBindings) {
        attach(binding);
    }
}

console.warn("[ReclaimTS] runtime started");
