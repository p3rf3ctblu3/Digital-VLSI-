library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity DebayeringFilter is
    generic (
        N: integer := 128
    );
    Port ( 
        clk, rst_n: in std_logic;
        new_image, valid_in: in std_logic;
        pixel: in std_logic_vector(7 downto 0);
        image_finished, valid_out: out std_logic;
        R, G, B: out std_logic_vector (7 downto 0)
    );
end DebayeringFilter;

architecture Behavioral of DebayeringFilter is
    
    component ControlUnit is
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
    end component;
        
    component SerialToParallel is 
        generic (
            N: integer
        );
        Port ( 
            clk: in std_logic;
            rst: in std_logic;
            fifo_we: in std_logic;
            pixel: in std_logic_vector(7 downto 0);
    
            -- 3x3 DFF grid
            q00 : out std_logic_vector(7 downto 0);
            q01 : out std_logic_vector(7 downto 0);
            q02 : out std_logic_vector(7 downto 0);
            q10 : out std_logic_vector(7 downto 0);
            q11 : out std_logic_vector(7 downto 0);
            q12 : out std_logic_vector(7 downto 0);
            q20 : out std_logic_vector(7 downto 0);
            q21 : out std_logic_vector(7 downto 0);
            q22 : out std_logic_vector(7 downto 0)
        );
    end component;
        
        
    component Demosaicing is 
        generic (
            N: integer
        );
        Port (
            clk: in std_logic;
            rst_n: in std_logic; --reset by zeroing the RGB vectors 
            demosaicing_en: in std_logic;
            
            -- OPTIONALLY, PASS DFFS DIAGONALLY IN THE WHOLE DEBAYER UNIT
    
            q00 : in std_logic_vector(7 downto 0);
            q01 : in std_logic_vector(7 downto 0);
            q02 : in std_logic_vector(7 downto 0);
            q10 : in std_logic_vector(7 downto 0);
            q11 : in std_logic_vector(7 downto 0);
            q12 : in std_logic_vector(7 downto 0);
            q20 : in std_logic_vector(7 downto 0);
            q21 : in std_logic_vector(7 downto 0);
            q22 : in std_logic_vector(7 downto 0);
    
            R: out std_logic_vector(7 downto 0);
            G: out std_logic_vector(7 downto 0);
            B: out std_logic_vector(7 downto 0)
        );
    end component; 

    signal fifo_we, demosaicing_en : std_logic;
    signal fifo_pixel : std_logic_vector (7 downto 0);
    signal rst: std_logic;
    signal q00, q01, q02, q10, q11, q12, q20, q21, q22: std_logic_vector(7 downto 0); 
    
begin

    ControlUnit_instance: ControlUnit
        generic map (
            N => N
        )
        port map (
            clk => clk,
            rst_n => rst_n,
            new_image => new_image,
            valid_in => valid_in,
            pixel => pixel,
            fifo_pixel => fifo_pixel,
            fifo_we => fifo_we, 
            demosaicing_en => demosaicing_en,
            image_finished => image_finished,
            valid_out => valid_out
        );
        
    rst <= not rst_n;

    S2P: SerialToParallel 
        generic map (
            N => N
        )
        port map (
            clk => clk,
            rst => rst,
            fifo_we => fifo_we,
            pixel => fifo_pixel,
            q00 => q00,
            q01 => q01,
            q02 => q02, 
            q10 => q10, 
            q11 => q11,
            q12 => q12,
            q20 => q20,
            q21 => q21,
            q22 => q22
        );

    Demosaicing_instance: Demosaicing 
        generic map (
            N => N
        )
        port map (
            clk => clk,
            rst_n => rst_n,
            demosaicing_en => demosaicing_en,
            q00 => q00,
            q01 => q01,
            q02 => q02, 
            q10 => q10, 
            q11 => q11,
            q12 => q12,
            q20 => q20,
            q21 => q21,
            q22 => q22,
            R => R,
            G => G,
            B => B  
        );

end Behavioral;
