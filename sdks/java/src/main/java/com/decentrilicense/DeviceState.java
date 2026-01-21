package com.decentrilicense;

/**
 * Device State Enum
 */
public enum DeviceState {
    IDLE(0),
    DISCOVERING(1),
    ELECTING(2),
    COORDINATOR(3),
    FOLLOWER(4);
    
    private final int value;
    
    DeviceState(int value) {
        this.value = value;
    }
    
    public int getValue() {
        return value;
    }
    
    public static DeviceState fromInt(int value) {
        for (DeviceState state : DeviceState.values()) {
            if (state.getValue() == value) {
                return state;
            }
        }
        return IDLE; // Default
    }
}