package app.organicmaps.sdk.bookmarks.data;

/**
 * represents the details of the socket available on a particular charging station
 *
 */
public record ChargeSocketDescriptor(String type, int count, double power) {

    /**
     * Some charge sockets have the same visuals as other sockets, even though they are different and are tagged
     * differently in OSM. This method returns the 'visual' type that should be used for the socket.
     *
     * @return the 'equivalent' visual style that should be used for this socket
     */
    public String visualType() {
        if (type.equals("typee")) {
            return "schuko";
        }
        return type;
    }

    /**
     * For some sockets (eg, domestic sockets), the power is usually not provided, as it is 'implicit'
     *
     * @return true if this socket type does not require displaying the power
     */
    public Boolean ignorePower() {
        return type.equals("typee") || type.equals("schuko");
    }

}
