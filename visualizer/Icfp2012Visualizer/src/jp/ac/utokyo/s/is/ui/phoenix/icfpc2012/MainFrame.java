package jp.ac.utokyo.s.is.ui.phoenix.icfpc2012;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;

import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;

public class MainFrame extends JFrame {
    private final MainPanel panel;
    private final JFileChooser fileChooser;

    public MainFrame() {
        panel = new MainPanel();
        fileChooser = new JFileChooser(new File("map"));
        add(panel);
        setTitle("ICFPC2012");
        setJMenuBar(createMenuBar());
        setDefaultCloseOperation(EXIT_ON_CLOSE);
    }

    public MainPanel getMainPanel() {
        return panel;
    }

    public Dimension getPreferredSize() {
        return new Dimension(640, 480);
    }

    private JMenuBar createMenuBar() {
        JMenuBar menuBar = new JMenuBar();
        {
            JMenu fileMenu = new JMenu("File");
            {
                JMenuItem item = new JMenuItem("Open");
                item.addActionListener(new ActionListener() {
                    @Override
                    public void actionPerformed(ActionEvent event) {
                        if (fileChooser.showOpenDialog(MainFrame.this) == JFileChooser.APPROVE_OPTION) {
                            try {
                                MapData mapData = new MapData(fileChooser.getSelectedFile());
                                MainFrame.this.getMainPanel().setMap(mapData);
                            } catch (IOException e) {
                                JOptionPane.showMessageDialog(MainFrame.this, e.toString(), "Failed to open the file", JOptionPane.ERROR_MESSAGE);
                            }
                        }
                        MainFrame.this.repaint();
                    }
                });
                fileMenu.add(item);
            }
            fileMenu.addSeparator();
            {
                JMenuItem item = new JMenu("Exit");
                item.addActionListener(new ActionListener() {
                    @Override
                    public void actionPerformed(ActionEvent e) {
                        System.exit(0);
                    }
                });
                fileMenu.add(item);
            }
            menuBar.add(fileMenu);
        }
        return menuBar;
    }
    
    
    public static void main(String[] args) {
        final MainFrame frame = new MainFrame();
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                frame.pack();
                frame.setVisible(true);
            }
        });
    }
}
