"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const ScriptController_1 = require("../Runtime/ScriptController");
const ScriptRegistry_1 = require("../Runtime/ScriptRegistry");
class EnemyPolicy extends ScriptController_1.ReclaimScriptController {
    onEvent(event) {
        switch (event.name) {
            case "Lifecycle.BeginPlay":
                break;
            default:
                break;
        }
    }
}
(0, ScriptRegistry_1.registerScriptController)("AI.Enemy", binding => new EnemyPolicy(binding));
