import { ReclaimScriptController, ScriptEvent } from "../Runtime/ScriptController";
import { registerScriptController } from "../Runtime/ScriptRegistry";

class PlayerCharacterPolicy extends ReclaimScriptController {
    override onEvent(event: ScriptEvent): void {
        switch (event.name) {
            case "Lifecycle.BeginPlay":
                break;
            default:
                break;
        }
    }
}

registerScriptController("Player.Character", binding => new PlayerCharacterPolicy(binding));
