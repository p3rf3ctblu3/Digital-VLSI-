library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.ALL;

entity ControlUnit is
    generic (
        N: integer -- STARTING POINT TO DEBUG 
    );
    Port ( 
        clk, rst_n: in std_logic;
        new_image, valid_in: in std_logic;
        pixel: in std_logic_vector(7 downto 0);

        fifo_pixel: out std_logic_vector(7 downto 0);
        fifo_we: out std_logic;

        demosaicing_en: out std_logic;

        image_finished, valid_out: out std_logic
    );
end ControlUnit;

architecture Behavioral of ControlUnit is
    -- FSM State Enumerations
    type t_State is (IDLE, PROCESSING);
    signal State : t_state; 

    signal counter : integer range 0 to (N*N + 2*N) := 0; 

begin
    
    process(clk) is
    begin
        if rising_edge(clk) then
            if rst_n = '0' then 
                image_finished <= '0';
                valid_out <= '0';

                demosaicing_en <= '0';
                fifo_we <= '0';

                counter <= 0;

                State <= IDLE; 
            else    
                case State is 
                    when IDLE =>
                        if (new_image = '1' and valid_in ='1') then
                            fifo_we <= '1';
                            fifo_pixel <= pixel;
                            State <= PROCESSING; 
                        end if;
                        -- else do nothing

                    when PROCESSING => 
                        -- FILLING 
                        if (counter < N*N + 2*N) then 
                            fifo_pixel <= pixel; -- pixel delayed by one clock cycle since next clock cycle it changes
                        else
                            fifo_we <= '0';
                        end if;

                        if (counter = (2*N + 2)) then 
                            demosaicing_en <= '1'; -- start demosaicing when the first 2 rows have been filled
                        elsif (counter = (2*N + 3)) then  
                            valid_out <= '1';      -- first pixel ready 1 clock cycle after demosaicing starts
                        elsif (counter = (N*N + 2*N + 2)) then
                            demosaicing_en <= '0'; -- all computations are done 
                            image_finished <= '1';
                        elsif (counter = (N*N + 2*N + 3)) then 
                            valid_out <= '0';
                            image_finished <= '0';

                            State <= IDLE;         -- all pixels computed after N*N + 2N clock cycles
                        end if;

                        counter <= counter + 1;
                end case;
            end if;
        end if;
    end process;

end Behavioral;
