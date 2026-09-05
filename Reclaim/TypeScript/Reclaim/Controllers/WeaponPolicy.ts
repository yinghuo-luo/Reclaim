import { ReclaimScriptController, ScriptEvent } from "../Runtime/ScriptController";
import { registerScriptController } from "../Runtime/ScriptRegistry";

class WeaponPolicy extends ReclaimScriptController {
    override onEvent(event: ScriptEvent): void {
        switch (event.name) {
            case "Lifecycle.BeginPlay":
                break;
            default:
                break;
        }
    }
}

registerScriptController("Weapon.Runtime", binding => new WeaponPolicy(binding));
