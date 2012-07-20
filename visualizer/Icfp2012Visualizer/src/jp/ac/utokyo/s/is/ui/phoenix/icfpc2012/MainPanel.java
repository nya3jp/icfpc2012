package jp.ac.utokyo.s.is.ui.phoenix.icfpc2012;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Image;
import java.io.File;
import java.io.IOException;

import javax.imageio.ImageIO;
import javax.swing.JPanel;

public class MainPanel extends JPanel {
    private MapData map;
    private Image closedlift;
    private Image earth;
    private Image lambda;
    private Image lifter;
    private Image openlift;
    private Image rock;
    private Image wall;

    private static final int IMAGE_WIDTH = 42;
    private static final int IMAGE_HEIGHT = 42;

    public MainPanel() {
        try {
            closedlift = ImageIO.read(new File("images/closedlift.png"));
            earth = ImageIO.read(new File("images/earth.png"));
            lambda = ImageIO.read(new File("images/lambda.png"));
            lifter = ImageIO.read(new File("images/lifter.png"));
            openlift = ImageIO.read(new File("images/openlift.png"));
            rock = ImageIO.read(new File("images/rock.png"));
            wall = ImageIO.read(new File("images/wall.png"));
        } catch (IOException e) {
            throw new AssertionError(e);
        }
        setDoubleBuffered(true);
    }

    public void setMap(MapData map) {
        this.map = map;
    }

    protected void paintComponent(Graphics g) {
        if (map == null) {
            return;
        }

        g.setColor(Color.BLACK);
        g.fillRect(0, 0, getWidth(), getHeight());

        int width = map.getWidth();
        int height = map.getHeight();
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                switch (map.get(x, y)) {
                    case 'R':
                        g.drawImage(lifter, x * IMAGE_WIDTH, y * IMAGE_HEIGHT, null);
                        break;
                    case '*':
                        g.drawImage(rock, x * IMAGE_WIDTH, y * IMAGE_HEIGHT, null);
                        break;
                    case 'L':
                        g.drawImage(closedlift, x * IMAGE_WIDTH, y * IMAGE_HEIGHT, null);
                        break;
                    case '.':
                        g.drawImage(earth, x * IMAGE_WIDTH, y * IMAGE_HEIGHT, null);
                        break;
                    case '#':
                        g.drawImage(wall, x * IMAGE_WIDTH, y * IMAGE_HEIGHT, null);
                        break;
                    case '\\':
                        g.drawImage(lambda, x * IMAGE_WIDTH, y * IMAGE_HEIGHT, null);
                        break;
                    case 'O':
                        g.drawImage(openlift, x * IMAGE_WIDTH, y * IMAGE_HEIGHT, null);
                        break;
                    case ' ':
                        break;
                }
            }
        }
    }
}
