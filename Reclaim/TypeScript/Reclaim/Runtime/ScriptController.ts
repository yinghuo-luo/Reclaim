export interface ScriptEvent {
    readonly name: string;
    readonly context: any;
    readonly payloadJson: string;
}

export interface ScriptController {
    onAttach?(): void;
    onEvent?(event: ScriptEvent): void;
    onDetach?(): void;
}

export type ScriptControllerFactory = (binding: any) => ScriptController;

/**
 * Base class for policy/orchestration code.
 * This is intentionally NOT an Unreal Actor replacement.
 */
export abstract class ReclaimScriptController implements ScriptController {
    constructor(protected readonly binding: any) {}

    onAttach(): void {}
    onEvent(_event: ScriptEvent): void {}
    onDetach(): void {}

    protected emitCommand(commandName: string, context: any = null, payload: unknown = {}): void {
        const payloadJson = typeof payload === "string" ? payload : JSON.stringify(payload ?? {});
        this.binding.EmitScriptCommand(commandName, context, payloadJson);
    }

    protected parsePayload<T>(payloadJson: string, fallback: T): T {
        if (!payloadJson) {
            return fallback;
        }
        try {
            return JSON.parse(payloadJson) as T;
        } catch {
            return fallback;
        }
    }
}
