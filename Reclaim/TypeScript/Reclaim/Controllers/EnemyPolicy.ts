import { ReclaimScriptController, ScriptEvent } from "../Runtime/ScriptController";
import { registerScriptController } from "../Runtime/ScriptRegistry";

class EnemyPolicy extends ReclaimScriptController {
    override onEvent(event: ScriptEvent): void {
        switch (event.name) {
            case "Lifecycle.BeginPlay":
                break;
            default:
                break;
        }
    }
}

registerScriptController("AI.Enemy", binding => new EnemyPolicy(binding));
